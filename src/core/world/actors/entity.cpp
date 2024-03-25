#include "pch.h"
#include "core/core.h"
#include "core/managers/animations.h"
#include "core/managers/ref.h"
#include "core/managers/renderer.h"
#include "core/model/model.h"
#include "core/world/actors/entity.h"

AEntity::AnimatedSkinBinding* AEntity::getAnimatedSkinBinding(const int32_t skinIndex) {
  if (skinIndex > -1 && skinIndex < m_animatedSkins.size()) {
    return &m_animatedSkins[skinIndex];
  }

  return nullptr;
}

AEntity::AnimatedNodeBinding* AEntity::getAnimatedNodeBinding(const int32_t nodeIndex) {
  for (auto& node : m_animatedNodes) {
    if (node.nodeIndex == nodeIndex) return &node;
  }

  return nullptr;
}

void AEntity::updateTransformBuffers() noexcept {
  for (auto& skin : m_animatedSkins) {
    int8_t* pSkinMemoryAddress = static_cast<int8_t*>(core::renderer.getSceneBuffers()
      ->skinTransformBuffer.allocInfo.pMappedData) + skin.skinTransformBufferOffset;

    int8_t* pPreviousSkinMemoryAddress = pSkinMemoryAddress + (sizeof(glm::mat4) * RE_MAXJOINTS);

    memcpy(pPreviousSkinMemoryAddress, pSkinMemoryAddress,
           sizeof(glm::mat4) * skin.transformBufferBlock.jointMatrices.size());

    memcpy(pSkinMemoryAddress, skin.transformBufferBlock.jointMatrices.data(),
           sizeof(glm::mat4) * skin.transformBufferBlock.jointMatrices.size());
  }

  for (auto& node : m_animatedNodes) {
    if (!node.requiresTransformBufferBlockUpdate && !node.isJustCreated) continue;

    int8_t* pPreviousNodeMemoryAddress =
      static_cast<int8_t*>(core::renderer.getSceneBuffers()
        ->nodeTransformBuffer.allocInfo.pMappedData) + node.nodeTransformBufferOffset;

    int8_t* pNodeMemoryAddress = pPreviousNodeMemoryAddress + (sizeof(glm::mat4));

    // Store previous frame node transformation matrix
    (node.isJustCreated)
      ? memcpy(pPreviousNodeMemoryAddress, &node.transformBufferBlock.nodeMatrix, sizeof(glm::mat4))
      : memcpy(pPreviousNodeMemoryAddress, pNodeMemoryAddress, sizeof(glm::mat4));

    // Copy node transform data for vertex shader (node matrix and joint count)
    memcpy(pNodeMemoryAddress, &node.transformBufferBlock.nodeMatrix,
      sizeof(glm::mat4) + sizeof(float));

    node.isJustCreated = false;
    node.requiresTransformBufferBlockUpdate = false;
  }
}

void AEntity::setModel(WModel* pModel) {
  if (!pModel) {
    RE_LOG(Error, "Failed to set model for actor '%s', received nullptr.",
           m_name.c_str());

    return;
  }

  m_pModel = pModel;
  m_animatedNodes.clear();

  std::vector<WModel::Node*>& pNodes = m_pModel->getAllNodes();

  // Create skin list
  const size_t skinCount = pModel->m_pSkins.size();
  m_animatedSkins.resize(skinCount);

  for (size_t skinIndex = 0; skinIndex < skinCount; ++skinIndex) {
    // Skins should be stored sequentially in the model, but this index check is here just in case they aren't
    const int32_t modelSkinIndex = pModel->m_pSkins[skinIndex]->index;
    m_animatedSkins[modelSkinIndex].skinIndex = pModel->m_pSkins[skinIndex]->index;
    m_animatedSkins[modelSkinIndex].transformBufferBlock.jointMatrices.resize(pModel->m_pSkins[skinIndex]->joints.size());

    // Set all matrices to identity to avoid NaN results in case no animation will be set
    for (auto& jointMatrix : m_animatedSkins[modelSkinIndex].transformBufferBlock.jointMatrices) {
      jointMatrix = glm::mat4(1.0f);
    }
  }

  // Create node list for all animated nodes
  for (auto& pNode : pNodes) {
    if (pNode->pMesh) {
      m_animatedNodes.emplace_back();
      m_animatedNodes.back().nodeIndex = pNode->index;

      if (pNode->pSkin) {
        m_animatedNodes.back().skinIndex = pNode->pSkin->index;
        m_animatedNodes.back().transformBufferBlock.jointCount = static_cast<float>(pNode->pSkin->joints.size());
        m_animatedNodes.back().pSkinBinding = getAnimatedSkinBinding(pNode->pSkin->index);
      }
    }
  }
}

WModel* AEntity::getModel() { return m_pModel; }

void AEntity::updateModel() {
  if (m_pModel && m_bindIndex != -1) {
    glm::mat4* pMemAddress = static_cast<glm::mat4*>(core::renderer.getSceneBuffers()
                             ->modelTransformBuffer.allocInfo.pMappedData) + m_rootTransformBufferIndex * 2;
    glm::mat4* pPreviousDataAddress = pMemAddress + 1;

    // Copy previous frame transform
    memcpy(pPreviousDataAddress, pMemAddress, sizeof(glm::mat4));

    const glm::mat4* pMatrix = &getModelTransformationMatrix(); 
    memcpy(pMemAddress, pMatrix, sizeof(glm::mat4));

    updateTransformBuffers();
  }
}

void AEntity::bindToRenderer() {
  if (m_bindIndex > -1) {
    RE_LOG(Warning, "Entity \"%s\" is already bound to renderer.",
           m_name.c_str());

    return;
  }

  // Register actor's root transform matrix
  bool isNew = core::animations.getOrRegisterActorOffsetIndex(this, m_rootTransformBufferIndex);
  m_rootTransformBufferOffset = sizeof(glm::mat4) * m_rootTransformBufferIndex;

  if (!isNew) {
    RE_LOG(
        Warning,
        "Attempted to register actor '%s' root matrix with root transformation "
        "buffer, but an actor is already registered.",
        m_name.c_str());
  }

  // Register actor's animated skin transform matrices
  for (auto& animatedSkin : m_animatedSkins) {
    core::animations.getOrRegisterSkinOffsetIndex(&animatedSkin, animatedSkin.skinTransformBufferIndex);
    animatedSkin.skinTransformBufferOffset = animatedSkin.skinTransformBufferIndex * config::scene::skinBlockSize;
  }

  // Register actor's animated node transform matrices
  for (auto& animatedNode : m_animatedNodes) {
    core::animations.getOrRegisterNodeOffsetIndex(&animatedNode, animatedNode.nodeTransformBufferIndex);
    animatedNode.nodeTransformBufferOffset = animatedNode.nodeTransformBufferIndex * config::scene::nodeBlockSize;
  }

  m_bindIndex = (int32_t)core::renderer.bindEntity(this);

  // Increase the number of times this model is bound to renderer and store the latest 0-based instance index
  m_pModel->m_instanceCount++;
  m_instanceIndex = m_pModel->m_instanceCount - 1;

  // Register entity with the scene graph
  core::ref.registerInstance(this);
}

void AEntity::unbindFromRenderer() {
  core::renderer.unbindEntity(m_bindIndex);
  m_bindIndex = -1;
}

int32_t AEntity::getRendererBindingIndex() { return m_bindIndex; }

void AEntity::setRendererBindingIndex(int32_t bindingIndex) {
  m_bindIndex = bindingIndex;
}

uint32_t AEntity::getRootTransformBufferIndex() {
  return m_rootTransformBufferIndex;
}

uint32_t AEntity::getRootTransformBufferOffset() {
  return m_rootTransformBufferOffset;
}

uint32_t AEntity::getNodeTransformBufferIndex(int32_t nodeIndex) {
  for (const auto& it : m_animatedNodes) {
    if (it.nodeIndex == nodeIndex) {
      return it.nodeTransformBufferIndex;
    }
  }

  RE_LOG(Warning, "Requested node '%d' is missing for actor '%s'.", nodeIndex,
    m_name.c_str());

  return -1;
}

uint32_t AEntity::getNodeTransformBufferOffset(int32_t nodeIndex) {
  for (const auto& it : m_animatedNodes) {
    if (it.nodeIndex == nodeIndex) {
      return it.nodeTransformBufferOffset;
    }
  }

  RE_LOG(Warning, "Requested node '%d' is missing for actor '%s'.", nodeIndex,
         m_name.c_str());

  return -1;
}

uint32_t AEntity::getSkinTransformBufferIndex(int32_t skinIndex) {
  if (skinIndex > -1 && skinIndex < m_animatedSkins.size()) {
    return m_animatedSkins[skinIndex].skinTransformBufferIndex;
  }

  return 0;
}

uint32_t AEntity::getSkinTransformBufferOffset(int32_t skinIndex) {
  if (skinIndex > -1 && skinIndex < m_animatedSkins.size()) {
     return m_animatedSkins[skinIndex].skinTransformBufferOffset;
  }

  return 0;
}

void AEntity::setInstancePrimitiveMaterial(const int32_t meshIndex, const int32_t primitiveIndex, const char* material) {
  if (m_instanceIndex == -1) {
    RE_LOG(Error, "Unable to set instance material. An actor '%s' must be bound to renderer first to receive a valid instance index.",
      m_name.c_str());
    return;
  }

  WPrimitive* pPrimitive = m_pModel->getPrimitive(meshIndex, primitiveIndex);

  if (!pPrimitive) {
    RE_LOG(Error,
      "Failed to set material '%s' for mesh %d, primitive %d of model "
      "'%s'. Couldn't get the required primitive.",
      material, meshIndex, primitiveIndex, m_name.c_str());
    return;
  }

  RMaterial* pMaterial = core::resources.getMaterial(material);

  if (!pMaterial) {
    RE_LOG(
      Error,
      "Failed to get material '%s' for mesh %d, primitive %d of model '%s'.",
      material, meshIndex, primitiveIndex, m_name.c_str());
    return;
  }

  pPrimitive->instanceData[m_instanceIndex].instanceBufferBlock.materialId = pMaterial->bufferIndex;

  // Update instance data buffer
  const uint32_t newMaterialId = pPrimitive->instanceData[m_instanceIndex].instanceBufferBlock.materialId;
  
  RBuffer stagingBuffer;
  core::renderer.createBuffer(EBufferType::STAGING, sizeof(uint32_t), stagingBuffer, (void*)&newMaterialId);

  VkBufferCopy copyRegion;
  copyRegion.srcOffset = 0;
  copyRegion.size = sizeof(uint32_t);
  copyRegion.dstOffset = sizeof(WPrimitiveDataEntry) * config::scene::uniquePrimitiveBudget
    + pPrimitive->instanceData[m_instanceIndex].instanceDataBufferOffset + sizeof(glm::vec4) * 2 + sizeof(uint32_t) * 3;

  core::renderer.copyBuffer(&stagingBuffer, &core::renderer.getSceneBuffers()->sourceDataBuffer, &copyRegion);

  vmaDestroyBuffer(core::renderer.memAlloc, stagingBuffer.buffer, stagingBuffer.allocation);
}

void AEntity::playAnimation(const std::string& name, const float speed,
  const bool loop, const bool isReversed) {
  for (const auto& boundAnimation : m_pModel->m_boundAnimations) {
    if (boundAnimation == name) {
      WAnimationInfo animationInfo;
      animationInfo.animationName = name;
      animationInfo.pEntity = this;
      animationInfo.speed = speed;
      animationInfo.loop = loop;

      if (isReversed) {
        const float endTime = animationInfo.endTime;
        animationInfo.endTime = animationInfo.startTime;
        animationInfo.startTime = endTime;
      }

      m_playingAnimations[name] = core::animations.addAnimationToQueue(&animationInfo);

      return;
    }
  }

  if (m_pModel->bindAnimation(name)) {
    playAnimation(name, speed, loop, isReversed);
  }
}

void AEntity::playAnimation(const WAnimationInfo* pAnimationInfo) {
  if (!pAnimationInfo) {
    RE_LOG(Error, "Failed to play animation for '%s', no data was provided.",
      m_name.c_str());
    return;
  }

  /*for (const auto& boundAnimation : m_boundAnimations) {
    if (boundAnimation == pAnimationInfo->animationName) {*/
      m_playingAnimations[pAnimationInfo->animationName] =
        core::animations.addAnimationToQueue(pAnimationInfo);

  //    return;
  //  }
  //}
}

bool AEntity::changeRenderPass(
  EDynamicRenderingPass newPass, const bool changeShadowPass, const uint32_t primitiveUID) {

  const uint32_t allowedPasses = (changeShadowPass)
    ? EDynamicRenderingPass::Shadow
    | EDynamicRenderingPass::ShadowDiscard

    : EDynamicRenderingPass::OpaqueCullBack
    | EDynamicRenderingPass::OpaqueCullNone
    | EDynamicRenderingPass::DiscardCullNone
    | EDynamicRenderingPass::BlendCullNone
    | EDynamicRenderingPass::MaskCullBack;

  const uint32_t clearMask = ~allowedPasses;

  // New rendering pass does not belong to the allowed passes if the result of this check is non-zero
  if (newPass & clearMask) {
    return false;
  }

  RBuffer stagingBuffer;
  core::renderer.createBuffer(EBufferType::STAGING, sizeof(WInstanceDataEntry), stagingBuffer, nullptr);

  VkBufferCopy bufferCopyRegion{};
  bufferCopyRegion.srcOffset = 0u;
  bufferCopyRegion.size = sizeof(uint32_t);

  // Change material render passes
  if (primitiveUID == -1) {
    for (auto& primitive : m_pModel->m_pLinearPrimitives) {
      primitive->pInitialMaterial->passFlags &= clearMask;
      primitive->pInitialMaterial->passFlags |= newPass;

      memcpy(stagingBuffer.allocInfo.pMappedData, &primitive->pInitialMaterial->passFlags, sizeof(uint32_t));

      bufferCopyRegion.dstOffset = 
        primitive->instanceData[m_instanceIndex].instanceDataBufferOffset + offsetof(WInstanceDataEntry, passFlags);
      core::renderer.copyBuffer(
        stagingBuffer.buffer, core::renderer.getSceneBuffers()->sourceDataBuffer.buffer, &bufferCopyRegion);
    }

    vmaDestroyBuffer(core::renderer.memAlloc, stagingBuffer.buffer, stagingBuffer.allocation);
    return true;
  }

  vmaDestroyBuffer(core::renderer.memAlloc, stagingBuffer.buffer, stagingBuffer.allocation);
  return true;
}