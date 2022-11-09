#include "pch.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"
#include "core/world/mesh/mesh.h"

void WMesh::setMemory() {
  VkDeviceSize bufferSize = vertices.size() * sizeof(RVertex);
  RBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  // assign ptr to vertex data
  stagingBuffer.pData = vertices.data();

  // create staging buffer and allocate CPU-accessible memory for it
  mgrGfx->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                       &stagingBuffer, stagingBufferMemory);

  // copy staging buffer to CPU accessible memory
  mgrGfx->copyToCPUAccessMemory(&stagingBuffer, stagingBufferMemory);

  // create GPU local vertex buffer
  mgrGfx->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, vertexBufferMemory);

  mgrGfx->copyBuffer(&stagingBuffer, &vertexBuffer);

  vkDestroyBuffer(mgrGfx->logicalDevice.device, stagingBuffer.buffer, nullptr);
  vkFreeMemory(mgrGfx->logicalDevice.device, stagingBufferMemory, nullptr);
};

void WMesh::destroy() {
  vkDestroyBuffer(mgrGfx->logicalDevice.device, vertexBuffer.buffer, nullptr);
  vkFreeMemory(mgrGfx->logicalDevice.device, vertexBufferMemory, nullptr);
}
