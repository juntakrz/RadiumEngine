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

void WMesh::createVertexBuffer() {
  VmaAllocator memAlloc = mgrGfx->memAlloc;
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
  vbAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  vmaCreateBuffer(memAlloc, &vbInfo, &vbAllocInfo, &stagingVertexBuffer.buffer,
                  &stagingVertexBuffer.allocation,
                  &stagingVertexBuffer.allocInfo);

  // not using Map/Unmap functions due to VMA_ALLOCATION_CREATE_MAPPED_BIT flag
  memcpy(stagingVertexBuffer.allocInfo.pMappedData, vertices.data(),
         vertexBufferSize);

  // destination vertex buffer
  vbInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  vbAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  vbAllocInfo.flags = NULL;
  vbAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  vmaCreateBuffer(memAlloc, &vbInfo, &vbAllocInfo, &vertexBuffer.buffer,
                  &vertexBuffer.allocation, &vertexBuffer.allocInfo);

  VkBufferCopy copyInfo{};
  copyInfo.srcOffset = 0;
  copyInfo.dstOffset = 0;
  copyInfo.size = vertexBufferSize;

  mgrGfx->copyBuffer(&stagingVertexBuffer, &vertexBuffer, &copyInfo);

  vmaDestroyBuffer(mgrGfx->memAlloc, stagingVertexBuffer.buffer,
                   stagingVertexBuffer.allocation);
}

void WMesh::createIndexBuffer() {
  VmaAllocator memAlloc = mgrGfx->memAlloc;
  const VkDeviceSize indexBufferSize = indices.size() * sizeof(indices[0]);

  // staging index buffer
  VkBufferCreateInfo ibInfo{};
  ibInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  ibInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  ibInfo.size = indexBufferSize;
  ibInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

  std::vector<uint32_t> queueFamilyIndices = {
      (uint32_t)mgrGfx->physicalDevice.queueFamilyIndices.graphics.at(0),
      (uint32_t)mgrGfx->physicalDevice.queueFamilyIndices.transfer.at(0)};

  ibInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  ibInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

  VmaAllocationCreateInfo ibAllocInfo{};
  ibAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  ibAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  vmaCreateBuffer(memAlloc, &ibInfo, &ibAllocInfo, &stagingIndexBuffer.buffer,
                  &stagingIndexBuffer.allocation,
                  &stagingIndexBuffer.allocInfo);

  // not using Map/Unmap functions due to VMA_ALLOCATION_CREATE_MAPPED_BIT flag
  memcpy(stagingIndexBuffer.allocInfo.pMappedData, indices.data(),
         indexBufferSize);

  // destination index buffer
  ibInfo.usage =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  ibAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  ibAllocInfo.flags = NULL;
  ibAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  vmaCreateBuffer(memAlloc, &ibInfo, &ibAllocInfo, &indexBuffer.buffer,
                  &indexBuffer.allocation, &indexBuffer.allocInfo);

  VkBufferCopy copyInfo{};
  copyInfo.srcOffset = 0;
  copyInfo.dstOffset = 0;
  copyInfo.size = indexBufferSize;

  mgrGfx->copyBuffer(&stagingIndexBuffer, &indexBuffer, &copyInfo, 1u);

  vmaDestroyBuffer(mgrGfx->memAlloc, stagingIndexBuffer.buffer,
                   stagingIndexBuffer.allocation);
}

void WMesh::allocateMemory() {
  createVertexBuffer();
  createIndexBuffer();
}

void WMesh::destroy() {
  vmaDestroyBuffer(mgrGfx->memAlloc, vertexBuffer.buffer,
                   vertexBuffer.allocation);
  vmaDestroyBuffer(mgrGfx->memAlloc, indexBuffer.buffer,
                   indexBuffer.allocation);
}