#include "pch.h"
#include "core/core.h"
#include "core/managers/animations.h"
#include "core/managers/renderer.h"
#include "core/model/model.h"
#include "core/world/actors/entity.h"

void AEntity::setModel(WModel* pModel) {
  if (!pModel) {
    RE_LOG(Error, "Failed to set model for actor '%s', received nullptr.",
           m_name.c_str());

    return;
  }

  m_pModel = pModel;
  m_animatedNodes.clear();

  std::vector<WModel::Node*>& pNodes = m_pModel->getAllNodes();

  // create node list for all animated nodes
  for (auto& pNode : pNodes) {
    if (pNode->pMesh) {
      m_animatedNodes.emplace_back();
      m_animatedNodes.back().nodeIndex = pNode->index;
    }
  }
}

WModel* AEntity::getModel() { return m_pModel; }

void AEntity::updateModel() {
  if (m_pModel && m_bindIndex != -1) {
    uint32_t* pMemAddress =
        static_cast<uint32_t*>(
            core::renderer.getSceneBuffers()
                ->rootTransformBuffer.allocInfo.pMappedData) +
        m_rootTransformBufferOffset;

    const glm::mat4* pMatrix = &getRootTransformationMatrix(); 

    memcpy(pMemAddress, pMatrix, sizeof(glm::mat4));

    for (auto& it : m_animatedNodes) {
      m_pModel->updateNodeTransformBuffer(
          it.nodeIndex, static_cast<uint32_t>(it.nodeTransformBufferOffset));
    }
  }
}

void AEntity::bindToRenderer() {
  if (m_bindIndex > -1) {
    RE_LOG(Warning, "Entity \"%s\" is already bound to renderer.",
           m_name.c_str());

    return;
  }

  m_bindIndex = (int32_t)core::renderer.bindEntity(this);

  // register actor's root transform matrix
  bool isNew = core::animations.getOrRegisterActorOffsetIndex(
      this, m_rootTransformBufferIndex);
  m_rootTransformBufferOffset = sizeof(glm::mat4) * m_rootTransformBufferIndex;

  if (!isNew) {
    RE_LOG(
        Warning,
        "Attempted to register actor '%s' root matrix with root transformation "
        "buffer, but an actor is already registered.",
        m_name.c_str());
  }

  // register actor's animated node transform matrices
  for (auto& animatedNode : m_animatedNodes) {
    core::animations.getOrRegisterNodeOffsetIndex(
        &animatedNode, animatedNode.nodeTransformBufferIndex);
    animatedNode.nodeTransformBufferOffset =
        sizeof(glm::mat4) * 2 * animatedNode.nodeTransformBufferIndex;
  }
}

void AEntity::unbindFromRenderer() {
  core::renderer.unbindEntity(m_bindIndex);
  m_bindIndex = -1;
}

int32_t AEntity::getRendererBindingIndex() { return m_bindIndex; }

void AEntity::setRendererBindingIndex(int32_t bindingIndex) {
  m_bindIndex = bindingIndex;
}

uint32_t AEntity::getRootTransformBufferOffset() {
  return static_cast<uint32_t>(m_rootTransformBufferOffset);
}

uint32_t AEntity::getNodeTransformBufferOffset(int32_t nodeIndex) {
  for (const auto& it : m_animatedNodes) {
    if (it.nodeIndex == nodeIndex) {
      return static_cast<uint32_t>(it.nodeTransformBufferOffset);
    }
  }

  RE_LOG(Warning, "Requested node '%d' is missing for actor '%s'.", nodeIndex,
         m_name.c_str());

  return -1;
}
