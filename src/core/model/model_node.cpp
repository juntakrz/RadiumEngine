#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/model/animation.h"
#include "core/model/model.h"

WModel::Node::Node(WModel::Node* pParent, uint32_t index,
                   const std::string& name)
    : pParentNode(pParent), index(index), name(name) {}

TResult WModel::Node::allocateMeshBuffer() {
  if (!pMesh) {
    RE_LOG(Error, "Failed to allocate mesh buffer - mesh was not created yet.");
    return RE_ERROR;
  }

  // prepare buffer and memory to store model/mesh/joints transformation matrix
  VkDeviceSize bufferSize = sizeof(pMesh->uniformBlock);
  core::renderer.createBuffer(EBufferMode::CPU_UNIFORM, bufferSize,
                              pMesh->uniformBufferData.uniformBuffer, nullptr);
  pMesh->uniformBufferData.descriptorBufferInfo = {
      pMesh->uniformBufferData.uniformBuffer.buffer, 0, bufferSize};

  return RE_OK;
}

void WModel::Node::destroyMeshBuffer() {
  if (!pMesh) {
    return;
  }

  vmaDestroyBuffer(core::renderer.memAlloc,
                   pMesh->uniformBufferData.uniformBuffer.buffer,
                   pMesh->uniformBufferData.uniformBuffer.allocation);
}

void WModel::Node::setNodeDescriptorSet(bool updateChildren) {
  if (!pMesh) {
    // most likely a joint node
    return;
  }

  VkDevice device = core::renderer.logicalDevice.device;

  // create node/mesh descriptor set
  VkDescriptorSetLayout layoutMesh =
      core::renderer.getDescriptorSetLayout(EDescriptorSetLayout::Mesh);

  VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
  descriptorSetAllocInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocInfo.descriptorPool = core::renderer.getDescriptorPool();
  descriptorSetAllocInfo.pSetLayouts = &layoutMesh;
  descriptorSetAllocInfo.descriptorSetCount = 1;

  VkResult result;
  if ((result = vkAllocateDescriptorSets(
         device, &descriptorSetAllocInfo,
          &pMesh->uniformBufferData.descriptorSet)) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create node descriptor set. Vulkan error %d.",
           result);
    return;
  }

  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.dstSet = pMesh->uniformBufferData.descriptorSet;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.pBufferInfo = &pMesh->uniformBufferData.descriptorBufferInfo;

  vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

  if (updateChildren) {
    for (auto& child : pChildren) {
      child->setNodeDescriptorSet(updateChildren);
    }
  }
}

void WModel::Node::propagateTransformation(const glm::mat4& accumulatedMatrix) {
  transformedNodeMatrix = accumulatedMatrix * getLocalMatrix();

  for (auto& child : pChildren) {
    child->propagateTransformation(transformedNodeMatrix);
  }
}

void WModel::Node::updateStagingNodeMatrices(const glm::mat4& modelMatrix,
                                             WAnimation* pOutAnimation) {
  if (pMesh) {
    pMesh->uniformBlock.rootMatrix = modelMatrix;
    pMesh->uniformBlock.nodeMatrix = transformedNodeMatrix;

    // node has mesh, store a reference to it in an animation
    pOutAnimation->addNodeReference(this->name, this->index);

    if (pSkin) {
      // Update joint matrices
      glm::mat4 inverseTransform = glm::inverse(transformedNodeMatrix);
      size_t numJoints = std::min((uint32_t)pSkin->joints.size(), RE_MAXJOINTS);
      for (size_t i = 0; i < numJoints; i++) {
        Node* pJointNode = pSkin->joints[i];
        glm::mat4 jointMatrix =
            pJointNode->transformedNodeMatrix * pSkin->inverseBindMatrices[i];
        jointMatrix = inverseTransform * jointMatrix;
        pMesh->uniformBlock.jointMatrices[i] = jointMatrix;
      }
      pMesh->uniformBlock.jointCount = (float)numJoints;
      memcpy(pMesh->uniformBufferData.uniformBuffer.allocInfo.pMappedData,
             &pMesh->uniformBlock, sizeof(pMesh->uniformBlock));
    } else {
      memcpy(pMesh->uniformBufferData.uniformBuffer.allocInfo.pMappedData,
             &pMesh->uniformBlock, sizeof(glm::mat4) * 2u);
    }
  }

  for (auto& pChild : pChildren) {
    pChild->updateStagingNodeMatrices(modelMatrix, pOutAnimation);
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

  pNode->pMesh = std::make_unique<WModel::Mesh>();
  pNode->allocateMeshBuffer();

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

    pNode->destroyMeshBuffer();
    pNode->pChildren.clear();
    pNode->pMesh->pPrimitives.clear();
    pNode->pMesh.reset();
  }

  pNode.reset();
}

WModel::Node* WModel::getNode(uint32_t index) noexcept {
  for (WModel::Node* it : m_pLinearNodes) {
    if (it->index == index) {
      return it;
    }
  }

  RE_LOG(Error, "Node with index %d not found for the model \"%s\".", index,
         m_name.c_str());
  return nullptr;
}