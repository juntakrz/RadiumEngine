#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/world/model/model.h"

WModel::Node::Node(WModel::Node* pParent, uint32_t index,
                   const std::string& name)
    : pParentNode(pParent), index(index), name(name) {}

TResult WModel::Node::allocateMeshBuffer() {
  if (!pMesh) {
    RE_LOG(Error, "Failed to allocate mesh buffer - mesh was not created yet.");
    return RE_ERROR;
  }

  // prepare buffer and memory to store node transformation matrix
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

  VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
  descriptorSetAllocInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocInfo.descriptorPool = core::renderer.getDescriptorPool();
  descriptorSetAllocInfo.pSetLayouts =
      &core::renderer.getDescriptorSetLayouts()->node;
  descriptorSetAllocInfo.descriptorSetCount = 1;

  if (vkAllocateDescriptorSets(
         device, &descriptorSetAllocInfo,
          &pMesh->uniformBufferData.descriptorSet) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create node descriptor set.");
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

void WModel::Node::updateNode() {
  if (pMesh) {
    glm::mat4 matrix = getMatrix();

    if (pSkin) {
      pMesh->uniformBlock.meshMatrix = matrix;
      // Update join matrices
      glm::mat4 inverseTransform = glm::inverse(matrix);
      size_t numJoints = std::min((uint32_t)pSkin->joints.size(), RE_MAXJOINTS);
      for (size_t i = 0; i < numJoints; i++) {
        Node* pJointNode = pSkin->joints[i];
        glm::mat4 jointMatrix =
            pJointNode->getMatrix() * pSkin->inverseBindMatrices[i];
        jointMatrix = inverseTransform * jointMatrix;
        pMesh->uniformBlock.jointMatrix[i] = jointMatrix;
      }
      pMesh->uniformBlock.jointCount = (float)numJoints;
      memcpy(pMesh->uniformBufferData.uniformBuffer.allocInfo.pMappedData,
             &pMesh->uniformBlock, sizeof(pMesh->uniformBlock));
    } else {
      memcpy(pMesh->uniformBufferData.uniformBuffer.allocInfo.pMappedData,
             &matrix, sizeof(glm::mat4));
    }
  }

  for (auto& pChild : pChildren) {
    pChild->updateNode();
  }
}

void WModel::Node::renderNode(VkCommandBuffer cmdBuffer, EAlphaMode alphaMode,
                              bool doubleSided, REntityBindInfo* pModelInfo) {
  if (pMesh) {
    uint32_t idFrameInFlight = core::renderer.getFrameInFlightIndex();

    for (const auto& primitive : pMesh->pPrimitives) {
      if (primitive->pMaterial->alphaMode != alphaMode ||
          primitive->pMaterial->doubleSided != doubleSided) {
        // does not belong to the current pipeline
        return;
      }

      // per primitive sets, will be bound at 1, because per frame set is bound
      // at 0
      const std::vector<VkDescriptorSet> descriptorSets = {
          primitive->pMaterial->descriptorSet,
          pMesh->uniformBufferData.descriptorSet};

      vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              core::renderer.getGraphicsPipelineLayout(), 1,
                              static_cast<uint32_t>(descriptorSets.size()),
                              descriptorSets.data(), 0, nullptr);

      vkCmdPushConstants(cmdBuffer, core::renderer.getGraphicsPipelineLayout(),
                         VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(RMaterialPCB),
                         &primitive->pMaterial->pushConstantBlock);

      int32_t vertexOffset =
          (int32_t)pModelInfo->vertexOffset + (int32_t)primitive->vertexOffset;
      uint32_t indexOffset = pModelInfo->indexOffset + primitive->indexOffset;

      vkCmdDrawIndexed(cmdBuffer, primitive->indexCount, 1, indexOffset,
                       vertexOffset, 0);
    }
  }

  // try rendering node children
  for (const auto& child : pChildren) {
    child->renderNode(cmdBuffer, alphaMode, doubleSided, pModelInfo);
  }
}

glm::mat4 WModel::Node::getLocalMatrix() {
  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
         glm::scale(glm::mat4(1.0f), scale) * nodeMatrix;
}

glm::mat4 WModel::Node::getMatrix() {
  glm::mat4 matrix = getLocalMatrix();
  Node* pParent = pParentNode;
  while (pParent) {
    matrix = pParent->getLocalMatrix() * matrix;
    pParent = pParent->pParentNode;
  }
  return matrix;
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
  pNode->nodeMatrix = glm::mat4(1.0f);

  pNode->pMesh = std::make_unique<WModel::Mesh>();
  pNode->allocateMeshBuffer();

  return pNode;
}

WModel::Node* WModel::getNode(uint32_t index) noexcept {
  for (auto it : m_pLinearNodes) {
    if (it->index == index) {
      return it;
    }
  }

  RE_LOG(Error, "Node with index %d not found for the model \"%s\".", index,
         m_name.c_str());
  return nullptr;
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