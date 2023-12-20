#include "pch.h"
#include "core/core.h"
#include "core/managers/animations.h"
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

void WModel::update(const glm::mat4& modelMatrix) noexcept {
  for (auto& node : getAllNodes()) {
    if (node->pMesh) {
      if (node->pSkin) {
        memcpy(
            node->pMesh->uniformBufferData.uniformBuffer.allocInfo.pMappedData,
            &node->pMesh->uniformBlock, sizeof(node->pMesh->uniformBlock));
      } else {
        memcpy(
            node->pMesh->uniformBufferData.uniformBuffer.allocInfo.pMappedData,
            &node->pMesh->uniformBlock, sizeof(glm::mat4) * 2u);
      }
    }
  }
}

void WModel::resetUniformBlockData() {
  for (auto& pNode : getAllNodes()) {
    this;
    if (pNode->pMesh) {
      pNode->pMesh->uniformBlock.nodeMatrix = glm::mat4(1.0f);

      if (pNode->pSkin) {
        for (int32_t i = 0; i < pNode->pSkin->joints.size(); ++i) {
          //pNode->pMesh->uniformBlock.jointMatrices[i] = glm::mat4(1.0f);
        }
      }
    }
  }

  update(glm::mat4(1.0f));
}

void WModel::setSceneBindingData(size_t vertexOffset, size_t indexOffset) {
  m_sceneVertexOffset = vertexOffset;
  m_sceneIndexOffset = indexOffset;
  m_isBoundToScene = true;
}

void WModel::clearSceneBindingData() {
  m_sceneVertexOffset = 0u;
  m_sceneIndexOffset = 0u;
  m_isBoundToScene = false;
}

void WModel::bindAnimation(const std::string& name) {
  for (const auto& boundAnimation : m_boundAnimations) {
    if (boundAnimation == name) {
      return;
    }
  }

  WAnimation* pAnimation = core::animations.getAnimation(name);

  if (!pAnimation) {
    RE_LOG(Error,
           "Failed to bind animation '%s' to model '%s'. Animation not found.",
           name.c_str(), m_name.c_str());
    return;
  }

  if (!pAnimation->validateModel(this)) {
    RE_LOG(Error,
           "Failed to bind animation '%s' to model '%s'. Model has an "
           "incompatible node structure.",
           name.c_str(), m_name.c_str());
    return;
  }

  m_boundAnimations.emplace_back(name);
}

void WModel::playAnimation(const std::string& name, const float speed,
                           const bool loop, const bool isReversed) {
  for (const auto& boundAnimation : m_boundAnimations) {
    if (boundAnimation == name) {
      WAnimationInfo animationInfo;
      animationInfo.animationName = name;
      animationInfo.pModel = this;
      animationInfo.speed = speed;
      animationInfo.loop = loop;

      if (isReversed) {
        const float endTime = animationInfo.endTime;
        animationInfo.endTime = animationInfo.startTime;
        animationInfo.startTime = endTime;
      }

      m_playingAnimations[name] =
          core::animations.addAnimationToQueue(&animationInfo);

      return;
    }
  }

  RE_LOG(Error, "Can't play animation '%s' as it was not bound to model '%s'.",
         name.c_str(), m_name.c_str());
}

void WModel::playAnimation(const WAnimationInfo* pAnimationInfo) {
  if (!pAnimationInfo) {
    RE_LOG(Error, "Failed to play animation for '%s', no data was provided.",
           m_name.c_str());
    return;
  }

  for (const auto& boundAnimation : m_boundAnimations) {
    if (boundAnimation == pAnimationInfo->animationName) {
      m_playingAnimations[pAnimationInfo->animationName] =
          core::animations.addAnimationToQueue(pAnimationInfo);

      return;
    }
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