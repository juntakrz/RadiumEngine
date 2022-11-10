#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"
#include "core/world/mesh/mesh.h"

void WMesh::allocateMemory() {
  VkBufferCreateInfo vertexBufferInfo{};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VmaAllocationCreateInfo allocCreateInfo{};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  vmaCreateBuffer(mgrGfx->memAlloc, &vertexBufferInfo, &allocCreateInfo,
                  &vertexBuffer.buffer, &vertexBuffer.allocation,
                  &vertexBuffer.allocInfo);
};

void WMesh::destroy() {
  //vkDestroyBuffer(mgrGfx->logicalDevice.device, vertexBuffer.buffer, nullptr);
  //vkFreeMemory(mgrGfx->logicalDevice.device, vertexBufferMemory, nullptr);
  vmaDestroyBuffer(mgrGfx->memAlloc, vertexBuffer.buffer,
                   vertexBuffer.allocation);
}
