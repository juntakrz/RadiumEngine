#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/ref.h"
#include "core/managers/renderer.h"
#include "core/managers/actors.h"
#include "core/world/actors/camera.h"

TResult core::MRenderer::createBuffer(EBufferMode mode, VkDeviceSize size, RBuffer& outBuffer, void* inData)
{
  switch ((uint8_t)mode) {
  case (uint8_t)EBufferMode::CPU_UNIFORM: {

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create CPU_UNIFORM buffer.");
      return RE_ERROR;
    }

    if (inData) {
      memcpy(outBuffer.allocInfo.pMappedData, inData, size);
    }

    return RE_OK;
  }

  case (uint8_t)EBufferMode::CPU_VERTEX: {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create CPU_VERTEX buffer.");
      return RE_ERROR;
    }

    if (inData) {
      void* pData = nullptr;
      if (vmaMapMemory(memAlloc, outBuffer.allocation, &pData) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to map memory for CPU_VERTEX buffer data.");
        return RE_ERROR;
      };
      memcpy(pData, inData, size);
      vmaUnmapMemory(memAlloc, outBuffer.allocation);
    }

    return RE_OK;
  }

  case (uint8_t)EBufferMode::CPU_INDEX: {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create CPU_INDEX buffer.");
      return RE_ERROR;
    }

    if (inData) {
      void* pData = nullptr;
      if (vmaMapMemory(memAlloc, outBuffer.allocation, &pData) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to map memory for CPU_INDEX buffer data.");
        return RE_ERROR;
      };
      memcpy(pData, inData, size);
      vmaUnmapMemory(memAlloc, outBuffer.allocation);
    }

    return RE_OK;
  }

  case (uint8_t)EBufferMode::DGPU_VERTEX: {
    // staging buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc{};
    VmaAllocationInfo stagingAllocInfo{};

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

    std::vector<uint32_t> queueFamilyIndices = {
        (uint32_t)physicalDevice.queueFamilyIndices.graphics.at(0),
        (uint32_t)physicalDevice.queueFamilyIndices.transfer.at(0) };

    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &stagingBuffer,
      &stagingAlloc, &stagingAllocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create staging buffer for DGPU_VERTEX mode.");
      return RE_ERROR;
    }

    void* pData = nullptr;
    if (vmaMapMemory(memAlloc, stagingAlloc, &pData) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to map memory for DGPU_VERTEX buffer data.");
      return RE_ERROR;
    };
    memcpy(pData, inData, size);
    vmaUnmapMemory(memAlloc, stagingAlloc);

    // destination buffer
    bufferCreateInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.flags = NULL;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
      return RE_ERROR;
    };

    VkBufferCopy copyInfo{};
    copyInfo.srcOffset = 0;
    copyInfo.dstOffset = 0;
    copyInfo.size = size;

    copyBuffer(stagingBuffer, outBuffer.buffer, &copyInfo);

    vmaDestroyBuffer(memAlloc, stagingBuffer, stagingAlloc);

    return RE_OK;
  }

  case (uint8_t)EBufferMode::DGPU_INDEX: {
    // staging buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc{};
    VmaAllocationInfo stagingAllocInfo{};

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

    std::vector<uint32_t> queueFamilyIndices = {
        (uint32_t)physicalDevice.queueFamilyIndices.graphics.at(0),
        (uint32_t)physicalDevice.queueFamilyIndices.transfer.at(0) };

    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &stagingBuffer,
      &stagingAlloc, &stagingAllocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create staging buffer for DGPU_VERTEX mode.");
      return RE_ERROR;
    }

    void* pData = nullptr;
    if (vmaMapMemory(memAlloc, stagingAlloc, &pData) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to map memory for DGPU_INDEX buffer data.");
      return RE_ERROR;
    };
    memcpy(pData, inData, size);
    vmaUnmapMemory(memAlloc, stagingAlloc);

    // destination vertex buffer
    bufferCreateInfo.usage =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.flags = NULL;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
      return RE_ERROR;
    };

    VkBufferCopy copyInfo{};
    copyInfo.srcOffset = 0;
    copyInfo.dstOffset = 0;
    copyInfo.size = size;

    copyBuffer(stagingBuffer, outBuffer.buffer, &copyInfo);

    vmaDestroyBuffer(memAlloc, stagingBuffer, stagingAlloc);

    return RE_OK;
  }
  case (uint8_t)EBufferMode::STAGING: {
    // staging buffer
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    std::vector<uint32_t> queueFamilyIndices = {
        (uint32_t)physicalDevice.queueFamilyIndices.graphics.at(0),
        (uint32_t)physicalDevice.queueFamilyIndices.transfer.at(0)};

    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
        static_cast<uint32_t>(queueFamilyIndices.size());

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &outBuffer.buffer,
                        &outBuffer.allocation, &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create staging buffer for STAGING mode.");
      return RE_ERROR;
    }

    if (inData) {
      void* pData = nullptr;
      if (vmaMapMemory(memAlloc, outBuffer.allocation, &pData) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to map memory for staging buffer data.");
        return RE_ERROR;
      };
      memcpy(pData, inData, size);
      vmaUnmapMemory(memAlloc, outBuffer.allocation);
    }

    return RE_OK;
  }
  }

  return RE_OK;
}

TResult core::MRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer& dstBuffer,
                              VkBufferCopy* copyRegion, uint32_t cmdBufferId) {
  if (cmdBufferId > MAX_TRANSFER_BUFFERS) {
    RE_LOG(Warning, "Invalid index of transfer buffer, using default.");
    cmdBufferId = 0;
  }

  VkCommandBufferBeginInfo cmdBufferBeginInfo{};
  cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command.buffersTransfer[cmdBufferId], &cmdBufferBeginInfo);

  vkCmdCopyBuffer(command.buffersTransfer[cmdBufferId], srcBuffer,
                  dstBuffer, 1, copyRegion);

  vkEndCommandBuffer(command.buffersTransfer[cmdBufferId]);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command.buffersTransfer[cmdBufferId];

  vkQueueSubmit(logicalDevice.queues.transfer, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(logicalDevice.queues.transfer);

  return RE_OK;
}

TResult core::MRenderer::copyBuffer(RBuffer* srcBuffer, RBuffer* dstBuffer,
                              VkBufferCopy* copyRegion, uint32_t cmdBufferId) {
  if (cmdBufferId > MAX_TRANSFER_BUFFERS) {
    RE_LOG(Warning, "Invalid index of transfer buffer, using default.");
    cmdBufferId = 0;
  }

  VkCommandBufferBeginInfo cmdBufferBeginInfo{};
  cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command.buffersTransfer[cmdBufferId], &cmdBufferBeginInfo);

  vkCmdCopyBuffer(command.buffersTransfer[cmdBufferId], srcBuffer->buffer,
                  dstBuffer->buffer, 1, copyRegion);

  vkEndCommandBuffer(command.buffersTransfer[cmdBufferId]);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command.buffersTransfer[cmdBufferId];

  vkQueueSubmit(logicalDevice.queues.transfer, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(logicalDevice.queues.transfer);

  return RE_OK;
}

void core::MRenderer::setCamera(const char* name) {
  if (ACamera* pCamera = core::ref.getActor(name)->getAs<ACamera>()) {
#ifndef NDEBUG
    RE_LOG(Log, "Selecting camera '%s'.", name);
#endif
    view.pActiveCamera = pCamera;
    return;
  }

  RE_LOG(Error, "Failed to set camera '%s' - not found.", name);
}

void core::MRenderer::setCamera(ACamera* pCamera) {
  if (pCamera != nullptr) {
    view.pActiveCamera = pCamera;
    return;
  }

  RE_LOG(Error, "Failed to set camera, received nullptr.");
}

ACamera* core::MRenderer::getCamera() { return view.pActiveCamera; }
