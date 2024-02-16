#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/model/animation.h"
#include "core/model/model.h"

WModel::Node::Node(WModel::Node* pParent, uint32_t index,
                   const std::string& name)
    : pParentNode(pParent), index(index), name(name) {}

void WModel::Node::propagateTransformation(const glm::mat4& accumulatedMatrix) {
  transformedNodeMatrix = accumulatedMatrix * getLocalMatrix();

  for (auto& child : pChildren) {
    child->propagateTransformation(transformedNodeMatrix);
  }
}

void WModel::Node::updateStagingNodeMatrices(WAnimation* pOutAnimation) {
  if (pMesh) {
    pMesh->stagingTransformBlock.nodeMatrix = transformedNodeMatrix;

    // node has mesh, store a reference to it in an animation
    pOutAnimation->addNodeReference(this->name, this->index);

    if (pSkin && pSkin->staging.recalculateSkinMatrices) {
      // Update joint matrices
      uint32_t numJoints = std::min(static_cast<uint32_t>(pSkin->joints.size()), RE_MAXJOINTS);

      for (uint32_t i = 0; i < numJoints; i++) {
        glm::mat4 jointMatrix = pSkin->joints[i]->transformedNodeMatrix *
                                pSkin->staging.inverseBindMatrices[i];
        pSkin->stagingTransformBlock.jointMatrices[i] = jointMatrix;
      }

      pSkin->staging.recalculateSkinMatrices = false;
    }
  }

  for (auto& pChild : pChildren) {
    pChild->updateStagingNodeMatrices(pOutAnimation);
  }
}

glm::mat4 WModel::Node::getLocalMatrix() {
  return glm::translate(glm::mat4(1.0f), staging.translation) *
         glm::mat4(staging.rotation) *
         glm::scale(glm::mat4(1.0f), staging.scale) * staging.nodeMatrix;
}

WModel::Node* WModel::createNode(WModel::Node* pParentNode, uint32_t nodeIndex,
                                 std::string nodeName) {
  WModel::Node* pNode = nullptr;

  if (pParentNode == nullptr) {
    m_pChildNodes.emplace_back(
        std::make_unique<WModel::Node>(pParentNode, nodeIndex, nodeName));
    pNode = m_pChildNodes.back().get();
  } else {
    pParentNode->pChildren.emplace_back(
        std::make_unique<WModel::Node>(pParentNode, nodeIndex, nodeName));
    pNode = pParentNode->pChildren.back().get();
  }

  if (!pNode) {
    RE_LOG(Error, "Trying to create the node '%s', but got nullptr.",
           nodeName.c_str());
    return nullptr;
  }

  pNode->name = nodeName;
  pNode->staging.nodeMatrix = glm::mat4(1.0f);

  return pNode;
}

void WModel::destroyNode(std::unique_ptr<WModel::Node>& pNode) {
  if (!pNode->pChildren.empty()) {
    for (auto& pChildNode : pNode->pChildren) {
      destroyNode(pChildNode);
    };
  }

  if (pNode->pMesh) {
    auto& primitives = pNode->pMesh->pPrimitives;
    for (auto& primitive : primitives) {
      if (primitive) {
        primitive.reset();
      }
    }

    pNode->pChildren.clear();
    pNode->pMesh->pPrimitives.clear();
    pNode->pMesh.reset();
  }

  pNode.reset();
}

WModel::Node* WModel::getNode(int32_t index) noexcept {
  for (WModel::Node* it : m_pLinearNodes) {
    if (it->index == index) {
      return it;
    }
  }

  RE_LOG(Error, "Node with index %d not found for the model \"%s\".", index,
         m_name.c_str());
  return nullptr;
}

WModel::Node* WModel::getNodeBySkinIndex(int32_t index) noexcept {
  for (auto& pNode : m_pLinearNodes) {
    if (pNode->skinIndex == index) return pNode;
  }

  return nullptr;
}

int32_t WModel::getSkinCount() noexcept {
  return static_cast<int32_t>(m_pSkins.size());
}

WModel::Skin* WModel::getSkin(int32_t skinIndex) noexcept {
  return m_pSkins[skinIndex].get();
}
