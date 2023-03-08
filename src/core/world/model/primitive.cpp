#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/world/model/primitive.h"

WPrimitive::WPrimitive() {
  vertexBuffer.buffer = VK_NULL_HANDLE;
  vertexBuffer.allocation = VK_NULL_HANDLE;

  indexBuffer.buffer = VK_NULL_HANDLE;
  indexBuffer.allocation = VK_NULL_HANDLE;
};

void WPrimitive::createVertexBuffer(const std::vector<RVertex>& vertexData) {
  VkDeviceSize size = vertexData.size() * sizeof(RVertex);
  core::renderer.createBuffer(EBufferMode::DGPU_VERTEX, size, vertexBuffer,
                              (void*)vertexData.data());
  vertexCount = static_cast<uint32_t>(vertexData.size());
}

void WPrimitive::createIndexBuffer(const std::vector<uint32_t>& indexData) {
  VkDeviceSize size = indexData.size() * sizeof(indexData[0]);
  core::renderer.createBuffer(EBufferMode::DGPU_INDEX, size, indexBuffer,
                              (void*)indexData.data());
  indexCount = static_cast<uint32_t>(indexData.size());
}

void WPrimitive::createBuffers(const std::vector<RVertex>& vertexData,
                          const std::vector<uint32_t>& indexData) {
  createVertexBuffer(vertexData);
  createIndexBuffer(indexData);
}

void WPrimitive::destroy() {
  vmaDestroyBuffer(core::renderer.memAlloc, vertexBuffer.buffer,
                   vertexBuffer.allocation);
  vmaDestroyBuffer(core::renderer.memAlloc, indexBuffer.buffer,
                   indexBuffer.allocation);
}

void WPrimitive::setBoundingBoxExtent(const glm::vec3& min,
                                      const glm::vec3& max) {
  extent.min = min;
  extent.max = max;
  extent.isValid = true;
}

const bool& WPrimitive::getBoundingBoxExtent(glm::vec3& outMin, glm::vec3& outMax) const {
  if (extent.isValid) {
    outMin = extent.min;
    outMax = extent.max;
  }
  return extent.isValid;
}

void WPrimitive::drawPrimitive(VkCommandBuffer cmdBuffer) {
  const uint32_t idFrameInFlight = core::renderer.getFrameInFlightIndex();
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);

  vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.buffer,
                       0, VK_INDEX_TYPE_UINT32);

  VkPipeline pipeline = VK_NULL_HANDLE;
  pipeline = pMaterial->doubleSided
                 ? core::renderer.getWorldPipelineSet().PBR_DS
                 : core::renderer.getWorldPipelineSet().PBR;

  if (pipeline != core::renderer.getBoundPipeline()) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  }

  const std::vector<VkDescriptorSet> descriptorSets = {
      core::renderer.getDescriptorSet(idFrameInFlight), pMaterial->descriptorSet};
  
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          core::renderer.getWorldPipelineLayout(), 0,
                          static_cast<uint32_t>(descriptorSets.size()),
                          descriptorSets.data(), 0, nullptr);

  vkCmdPushConstants(cmdBuffer, core::renderer.getWorldPipelineLayout(),
                     VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(RPushConstantBlock_Material),
                     &pMaterial->pushConstantBlock);

  vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
}
