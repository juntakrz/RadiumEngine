#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"
#include "core/world/mesh/mesh.h"

WMesh::WMesh() {
  stagingVertexBuffer.buffer = VK_NULL_HANDLE;
  stagingVertexBuffer.allocation = VK_NULL_HANDLE;

  vertexBuffer.buffer = VK_NULL_HANDLE;
  vertexBuffer.allocation = VK_NULL_HANDLE;

  stagingIndexBuffer.buffer = VK_NULL_HANDLE;
  stagingIndexBuffer.allocation = VK_NULL_HANDLE;

  indexBuffer.buffer = VK_NULL_HANDLE;
  indexBuffer.allocation = VK_NULL_HANDLE;
};

void WMesh::allocateMemory() {
  VmaAllocator memAlloc = mgrGfx->memAlloc;

  // 1. Vertex buffer
  const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(RVertex);

  // staging vertex buffer
  VkBufferCreateInfo vbInfo{};
  vbInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vbInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  vbInfo.size = vertexBufferSize;
  vbInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

  std::vector<uint32_t> queueFamilyIndices = {
      (uint32_t)mgrGfx->physicalDevice.queueFamilyIndices.graphics.at(0),
      (uint32_t)mgrGfx->physicalDevice.queueFamilyIndices.transfer.at(0)};
  vbInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  vbInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

  VmaAllocationCreateInfo vbAllocInfo{};
  vbAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  vbAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  vmaCreateBuffer(memAlloc, &vbInfo, &vbAllocInfo,
                  &stagingVertexBuffer.buffer, &stagingVertexBuffer.allocation,
                  &stagingVertexBuffer.allocInfo);

  // not using Map/Unmap functions due to VMA_ALLOCATION_CREATE_MAPPED_BIT flag
  memcpy(stagingVertexBuffer.allocInfo.pMappedData, vertices.data(), vertexBufferSize);

  // destination vertex buffer
  vbInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  vbAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  vbAllocInfo.flags = NULL;

  vmaCreateBuffer(memAlloc, &vbInfo, &vbAllocInfo,
                  &vertexBuffer.buffer, &vertexBuffer.allocation, &vertexBuffer.allocInfo);

  VkBufferCopy copyInfo{};
  copyInfo.srcOffset = 0;
  copyInfo.dstOffset = 0;
  copyInfo.size = vertexBufferSize;

  mgrGfx->copyBuffer(&stagingVertexBuffer, &vertexBuffer, &copyInfo);
}

void WMesh::destroy() {
  vmaDestroyBuffer(mgrGfx->memAlloc, stagingVertexBuffer.buffer,
                   stagingVertexBuffer.allocation);
  vmaDestroyBuffer(mgrGfx->memAlloc, vertexBuffer.buffer,
                   vertexBuffer.allocation);
}
