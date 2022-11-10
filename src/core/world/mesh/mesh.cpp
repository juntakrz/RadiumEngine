#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"
#include "core/world/mesh/mesh.h"

void WMesh::allocateMemory() {

  // 1. Vertex buffer

  // staging buffer before writing to VRAM
  const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(RVertex);

  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingBufferInfo.size = vertexBufferSize;

  VmaAllocationCreateInfo stagingAllocCreateInfo{};
  stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  stagingAllocCreateInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  vmaCreateBuffer(mgrGfx->memAlloc, &stagingBufferInfo, &stagingAllocCreateInfo,
                  &stagingBuffer.buffer, &stagingBuffer.allocation,
                  &stagingBuffer.allocInfo);

  // not using Map/Unmap functions due to VMA_ALLOCATION_CREATE_MAPPED_BIT flag
  memcpy(stagingBuffer.allocInfo.pMappedData, vertices.data(), vertexBufferSize);

  /*
  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.usage = 
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  stagingBufferInfo.size = vertexBufferSize;

  VmaAllocationCreateInfo stagingAllocCreateInfo{};
  stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  stagingAllocCreateInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  vmaCreateBuffer(mgrGfx->memAlloc, &stagingBufferInfo, &stagingAllocCreateInfo,
                  &vertexBuffer.buffer, &vertexBuffer.allocation, &vertexBuffer.allocInfo);

  // not using Map/Unmap functions due to VMA_ALLOCATION_CREATE_MAPPED_BIT flag
  memcpy(vertexBuffer.allocInfo.pMappedData, vertices.data(), vertexBufferSize);*/
};

void WMesh::destroy() {
  vmaDestroyBuffer(mgrGfx->memAlloc, stagingBuffer.buffer,
                   stagingBuffer.allocation);
  //vmaDestroyBuffer(mgrGfx->memAlloc, vertexBuffer.buffer,
    //               vertexBuffer.allocation);
  //vmaFreeMemory(mgrGfx->memAlloc, vertexBuffer.allocation);     // error here
}
