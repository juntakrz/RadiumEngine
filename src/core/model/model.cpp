#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/resources.h"
#include "core/managers/time.h"
#include "core/model/model.h"

TResult WModel::createStagingBuffers() {
  if (validateStagingData() != RE_OK) {
    RE_LOG(Error, "Staging data is invalid for model \"%s\".", m_name.c_str());
    return RE_ERROR; 
  }

  // create staging buffers for later copy to scene buffer
#ifndef NDEBUG
  RE_LOG(Log, "Creating staging buffers for this model.");
#endif

  VkDeviceSize vertexBufferSize = sizeof(RVertex) * staging.vertices.size();
  VkDeviceSize indexBufferSize = sizeof(uint32_t) * staging.indices.size();

  core::renderer.createBuffer(EBufferMode::STAGING, vertexBufferSize,
                              staging.vertexBuffer, staging.vertices.data());
  core::renderer.createBuffer(EBufferMode::STAGING, indexBufferSize,
                              staging.indexBuffer, staging.indices.data());

  // clear some of the raw staging data
  staging.pInModel = nullptr;
  staging.vertices.clear();
  staging.indices.clear();

  // mark that staging data still contains buffers
  staging.isClean = false;

  return RE_OK; 
}

TResult WModel::validateStagingData() {
  if (m_indexCount != staging.currentIndexOffset ||
      m_vertexCount != staging.currentVertexOffset ||
      staging.indices.size() != m_indexCount ||
      staging.vertices.size() != m_vertexCount) {
    return RE_ERROR;
  }
  return RE_OK;
}

void WModel::clearStagingData() {

  // this method was already called
  if (staging.isClean) {
    return;
  }

  staging.pInModel = nullptr;

  if (!staging.vertices.empty()) {
    staging.vertices.clear();
  }

  if (!staging.indices.empty()) {
    staging.indices.clear();
  }
  
  vmaDestroyBuffer(core::renderer.memAlloc, staging.vertexBuffer.buffer,
                     staging.vertexBuffer.allocation);
  vmaDestroyBuffer(core::renderer.memAlloc, staging.indexBuffer.buffer,
                     staging.indexBuffer.allocation);

  // all staging data is guaranteed to be empty
  staging.isClean = true;
}

void WModel::sortPrimitivesByMaterial() {
  std::vector<std::vector<WPrimitive*>> vectors;
  RMaterial* primitiveMaterial = nullptr;
  bool wasPlaced = false;

  for (const auto& primitive : m_pLinearPrimitives) {
    wasPlaced = false;
    primitiveMaterial = primitive->pMaterial;

    for (auto& vector : vectors) {
      if (vector[0]->pMaterial == primitiveMaterial) {
        vector.emplace_back(primitive);
        wasPlaced = true;
        break;
      }
    }

    if (!wasPlaced) {
      vectors.emplace_back(std::vector<WPrimitive*>{});
      vectors.back().emplace_back(primitive);
    }
  }

  uint32_t pos = 0u;
  for (const auto& srcVector : vectors) {
    for (const auto& srcPrimitive : srcVector) {
      m_pLinearPrimitives[pos] = srcPrimitive;
      ++pos;
    }
  }
}

void WModel::prepareStagingData() {
  if (m_vertexCount == 0 || m_indexCount == 0) {
    RE_LOG(Error,
           "Can't prepare local staging buffers for model \"%s\", nodes "
           "weren't parsed yet.");
    return;
  }

  staging.vertices.resize(m_vertexCount);
  staging.indices.resize(m_indexCount);
}

const char* WModel::getName() { return m_name.c_str(); }

bool WModel::Mesh::validateBoundingBoxExtent() {
  if (extent.min == extent.max) {
    extent.isValid = false;
    RE_LOG(Warning, "Bounding box of a mesh was invalidated.");
  }

  return extent.isValid;
}

const std::vector<WPrimitive*>& WModel::getPrimitives() {
  return m_pLinearPrimitives;
}

std::vector<uint32_t>& WModel::getPrimitiveBindsIndex() {
  return m_primitiveBindsIndex;
}

const std::vector<std::unique_ptr<WModel::Node>>& WModel::getRootNodes() noexcept {
  return m_pChildNodes;
}

std::vector<WModel::Node*>& WModel::getAllNodes() noexcept {
  return m_pLinearNodes;
}

void WModel::updateAnimation() {
  if (!m_animationSettings.enable) return;

  Animation& animation = m_animations[m_animationSettings.index];

  // iterate through transformation animation channels
  for (auto& channel : animation.channels) {
    AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
    if (sampler.timePoints.size() > sampler.outputsVec4.size()) {
      continue;
    }

    // iterate through sampler time points and retrieve vector for current channel
    for (size_t i = 0; i < sampler.timePoints.size() - 1; ++i) {
      if ((m_animationSettings.timePoint >= sampler.timePoints[i]) &&
          (m_animationSettings.timePoint <=
           sampler.timePoints[i + 1])) {
        float u = std::max(0.0f, m_animationSettings.timePoint -
                                     sampler.timePoints[i]) /
                  (sampler.timePoints[i + 1] - sampler.timePoints[i]);
        if (u <= 1.0f) {
          switch (channel.path) {
            case AnimationChannel::EPathType::TRANSLATION : {
              glm::vec4 translation = glm::mix(sampler.outputsVec4[i],
                                         sampler.outputsVec4[i + 1], u);
              channel.pTargetNode->translation = glm::vec3(translation);
              break;
            }
            case AnimationChannel::EPathType::SCALE : {
              glm::vec4 scaling = glm::mix(sampler.outputsVec4[i],
                                         sampler.outputsVec4[i + 1], u);
              channel.pTargetNode->scale = glm::vec3(scaling);
              break;
            }
            case AnimationChannel::EPathType::ROTATION: {
              glm::quat q1;
              q1.x = sampler.outputsVec4[i].x;
              q1.y = sampler.outputsVec4[i].y;
              q1.z = sampler.outputsVec4[i].z;
              q1.w = sampler.outputsVec4[i].w;
              glm::quat q2;
              q2.x = sampler.outputsVec4[i + 1].x;
              q2.y = sampler.outputsVec4[i + 1].y;
              q2.z = sampler.outputsVec4[i + 1].z;
              q2.w = sampler.outputsVec4[i + 1].w;
              channel.pTargetNode->rotation =
                  glm::normalize(glm::slerp(q1, q2, u));
              break;
            }
          }
        }
      }
    }

    // update animation time for next frame
    m_animationSettings.timePoint += core::time.getDeltaTime() * m_animationSettings.speed;

    if (m_animationSettings.timePoint >
            m_animations[m_animationSettings.index].end) {

      switch (m_animationSettings.loopCurrent) {
        case true: {
          m_animationSettings.timePoint = 0.0f;
          break;
        }
        case false: {
          m_animationSettings.timePoint = 0.0f;
          m_animationSettings.enable = false;
        }
      }
    }
  }
}

void WModel::playAnimation(const int32_t index, const float timePoint, const float speed, const bool loop) {
  // validate that parameters are correct and animations can be played
  if (m_animations.empty()) {
    m_animationSettings.enable = false;
    return;
  }

  if (index > m_animations.size() - 1 || index < 0) {
    RE_LOG(Error, "Invalid animation index %d for model '%s'.", index,
           m_name);
    m_animationSettings.enable = false;
    return;
  }

  // set values for updateAnimation method
  m_animationSettings.index = index;
  m_animationSettings.timePoint = timePoint;
  m_animationSettings.speed = speed;
  m_animationSettings.loopCurrent = loop;
  m_animationSettings.enable = true;
}

void WModel::playAnimation(const char* name, const float timePoint,
                           const float speed, const bool loop) {
  m_animationSettings.timePoint = timePoint;
  m_animationSettings.speed = speed;
  m_animationSettings.loopCurrent = loop;
  m_animationSettings.enable = true;
}

void WModel::update(const glm::mat4& modelMatrix) noexcept {
  // update skins
  //for (auto& skin : m_pSkins) {

  //}

  // update animation
  updateAnimation();

  // update node transformations
  for (auto& node : getRootNodes()) {
    // propagate node transformation accumulating from parent to children
    node->propagateTransformation();

    // set root and propagated transformation matrices and update joint matrices
    node->updateNodeMatrices(modelMatrix);

    //node->updateNode(modelMatrix);
  }
}

TResult WModel::clean() {
  if (m_pChildNodes.empty()) {
    RE_LOG(Warning,
           "Model '%s' can not be cleared. It may be already empty and ready "
           "for deletion.",
           m_name.c_str());

    return RE_WARNING;
  }

  for (auto& node : m_pChildNodes) {
    destroyNode(node);
  }

  for (auto& skin : m_pSkins) {
    skin.reset();
  }
  m_pSkins.clear();

  m_pChildNodes.clear();
  m_pLinearNodes.clear();
  m_pLinearPrimitives.clear();

  clearStagingData();

  RE_LOG(Log, "Model '%s' is prepared for deletion.", m_name.c_str());

  return RE_OK;
}