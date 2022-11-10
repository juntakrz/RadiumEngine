#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"
#include "core/world/mesh/mesh.h"

void WMesh::allocateMemory() {
  VmaAllocator memAlloc = mgrGfx->memAlloc;

  // 1. Vertex buffer
  const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(RVertex);

  // staging vertex buffer
  VkBufferCreateInfo svbInfo{};
  svbInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  svbInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  svbInfo.size = vertexBufferSize;
  svbInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo svbAllocInfo{};
  svbAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  svbAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  vmaCreateBuffer(memAlloc, &svbInfo, &svbAllocInfo,
                  &stagingVertexBuffer.buffer, &stagingVertexBuffer.allocation,
                  &stagingVertexBuffer.allocInfo);

  // not using Map/Unmap functions due to VMA_ALLOCATION_CREATE_MAPPED_BIT flag
  memcpy(stagingVertexBuffer.allocInfo.pMappedData, vertices.data(), vertexBufferSize);

  // destination vertex buffer
  VkBufferCreateInfo vertexBufferInfo{};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  vertexBufferInfo.size = vertexBufferSize;

  VmaAllocationCreateInfo vertexAllocCreateInfo{};
  vertexAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  vertexAllocCreateInfo.flags = NULL;

  vmaCreateBuffer(mgrGfx->memAlloc, &vertexBufferInfo, &vertexAllocCreateInfo,
                  &vertexBuffer.buffer, &vertexBuffer.allocation, &vertexBuffer.allocInfo);

  VkBufferCopy copyInfo{};
  copyInfo.srcOffset = 0;
  copyInfo.dstOffset = 0;
  copyInfo.size = vertexBufferSize;

  vkCmdCopyBuffer(*mgrGfx->getCmdBuffer(0), stagingBuffer.buffer,
                  vertexBuffer.buffer, 1, &copyInfo);
};

void WMesh::destroy() {
  vmaDestroyBuffer(mgrGfx->memAlloc, stagingBuffer.buffer,
                   stagingBuffer.allocation);
  //vmaDestroyBuffer(mgrGfx->memAlloc, vertexBuffer.buffer,
    //               vertexBuffer.allocation);
  //vmaFreeMemory(mgrGfx->memAlloc, vertexBuffer.allocation);     // error here
}
