#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/managers/mgraphics.h"

TResult MGraphics::createBuffer(EBCMode mode, VkDeviceSize size,
                                VkBuffer& outBuffer, VmaAllocation& outAlloc,
                                void* inData, VmaAllocationInfo* outAllocInfo) {
  switch ((uint8_t)mode) {
    case (uint8_t)EBCMode::CPU_UNIFORM: {
      VkBufferCreateInfo bcInfo{};
      bcInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bcInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      bcInfo.size = size;
      bcInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo allocInfo{};
      allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
      allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

      if (vmaCreateBuffer(memAlloc, &bcInfo, &allocInfo, &outBuffer, &outAlloc,
                          outAllocInfo) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to create CPU_UNIFORM buffer.");
        return RE_ERROR;
      }

      if (inData) {
        void* pData = nullptr;
        if (vmaMapMemory(memAlloc, outAlloc, &pData) != VK_SUCCESS) {
          RE_LOG(Error, "Failed to map memory for CPU_UNIFORM buffer data.");
          return RE_ERROR;
        };
        memcpy(outBuffer, inData, size);
        vmaUnmapMemory(memAlloc, outAlloc);
      }

      return RE_OK;
    }

    case (uint8_t)EBCMode::CPU_VERTEX: {
      VkBufferCreateInfo bcInfo{};
      bcInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bcInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      bcInfo.size = size;
      bcInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo allocInfo{};
      allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
      allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

      if (vmaCreateBuffer(memAlloc, &bcInfo, &allocInfo, &outBuffer, &outAlloc,
                          outAllocInfo) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to create CPU_VERTEX buffer.");
        return RE_ERROR;
      }

      if (inData) {
        void* pData = nullptr;
        if (vmaMapMemory(memAlloc, outAlloc, &pData) != VK_SUCCESS) {
          RE_LOG(Error, "Failed to map memory for CPU_VERTEX buffer data.");
          return RE_ERROR;
        };
        memcpy(pData, inData, size);
        vmaUnmapMemory(memAlloc, outAlloc);
      }

      return RE_OK;
    }

    case (uint8_t)EBCMode::CPU_INDEX: {
      VkBufferCreateInfo bcInfo{};
      bcInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bcInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      bcInfo.size = size;
      bcInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo allocInfo{};
      allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
      allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

      if (vmaCreateBuffer(memAlloc, &bcInfo, &allocInfo, &outBuffer, &outAlloc,
                          outAllocInfo) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to create CPU_INDEX buffer.");
        return RE_ERROR;
      }

      if (inData) {
        void* pData = nullptr;
        if (vmaMapMemory(memAlloc, outAlloc, &pData) != VK_SUCCESS) {
          RE_LOG(Error, "Failed to map memory for CPU_INDEX buffer data.");
          return RE_ERROR;
        };
        memcpy(pData, inData, size);
        vmaUnmapMemory(memAlloc, outAlloc);
      }

      return RE_OK;
    }

    case (uint8_t)EBCMode::DGPU_VERTEX: {
      // staging buffer
      VkBuffer stagingBuffer;
      VmaAllocation stagingAlloc{};
      VmaAllocationInfo stagingAllocInfo{};

      VkBufferCreateInfo bcInfo{};
      bcInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bcInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      bcInfo.size = size;
      bcInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

      std::vector<uint32_t> queueFamilyIndices = {
          (uint32_t)physicalDevice.queueFamilyIndices.graphics.at(0),
          (uint32_t)physicalDevice.queueFamilyIndices.transfer.at(0)};

      bcInfo.pQueueFamilyIndices = queueFamilyIndices.data();
      bcInfo.queueFamilyIndexCount =
          static_cast<uint32_t>(queueFamilyIndices.size());

      VmaAllocationCreateInfo allocInfo{};
      allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
      allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT;

      if (vmaCreateBuffer(memAlloc, &bcInfo, &allocInfo, &stagingBuffer,
                          &stagingAlloc, &stagingAllocInfo) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to create staging buffer with DGPU_VERTEX mode.");
        return RE_ERROR;
      }

      // not using Map/Unmap functions due to VMA_ALLOCATION_CREATE_MAPPED_BIT
      // flag
      memcpy(stagingAllocInfo.pMappedData, inData, size);

      // destination buffer
      bcInfo.usage =
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

      allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
      allocInfo.flags = NULL;
      allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

      if(vmaCreateBuffer(memAlloc, &bcInfo, &allocInfo, &outBuffer,
        &outAlloc, outAllocInfo) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
        return RE_ERROR;
      };

      VkBufferCopy copyInfo{};
      copyInfo.srcOffset = 0;
      copyInfo.dstOffset = 0;
      copyInfo.size = size;

      copyBuffer(stagingBuffer, outBuffer, &copyInfo);

      vmaDestroyBuffer(memAlloc, stagingBuffer,
                       stagingAlloc);

      return RE_OK;
    }

    case (uint8_t)EBCMode::DGPU_INDEX: {
      // staging buffer
      VkBuffer stagingBuffer;
      VmaAllocation stagingAlloc{};
      VmaAllocationInfo stagingAllocInfo{};

      VkBufferCreateInfo bcInfo{};
      bcInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bcInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      bcInfo.size = size;
      bcInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

      std::vector<uint32_t> queueFamilyIndices = {
          (uint32_t)physicalDevice.queueFamilyIndices.graphics.at(0),
          (uint32_t)physicalDevice.queueFamilyIndices.transfer.at(0)};

      bcInfo.pQueueFamilyIndices = queueFamilyIndices.data();
      bcInfo.queueFamilyIndexCount =
          static_cast<uint32_t>(queueFamilyIndices.size());

      VmaAllocationCreateInfo allocInfo{};
      allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
      allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT;

      if (vmaCreateBuffer(memAlloc, &bcInfo, &allocInfo, &stagingBuffer,
                          &stagingAlloc, &stagingAllocInfo) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to create staging buffer with DGPU_VERTEX mode.");
        return RE_ERROR;
      }

      // not using Map/Unmap functions due to VMA_ALLOCATION_CREATE_MAPPED_BIT
      // flag
      memcpy(stagingAllocInfo.pMappedData, inData, size);

      // destination vertex buffer
      bcInfo.usage =
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

      allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
      allocInfo.flags = NULL;
      allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

      if (vmaCreateBuffer(memAlloc, &bcInfo, &allocInfo, &outBuffer, &outAlloc,
                          outAllocInfo) != VK_SUCCESS) {
        RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
        return RE_ERROR;
      };

      VkBufferCopy copyInfo{};
      copyInfo.srcOffset = 0;
      copyInfo.dstOffset = 0;
      copyInfo.size = size;

      copyBuffer(stagingBuffer, outBuffer, &copyInfo);

      vmaDestroyBuffer(memAlloc, stagingBuffer, stagingAlloc);

      return RE_OK;
    }
  }

  return RE_OK;
}

TResult MGraphics::copyBuffer(VkBuffer srcBuffer, VkBuffer& dstBuffer,
                              VkBufferCopy* copyRegion, uint32_t cmdBufferId) {
  if (cmdBufferId > MAX_TRANSFER_BUFFERS) {
    RE_LOG(Warning, "Invalid index of transfer buffer, using default.");
    cmdBufferId = 0;
  }

  VkCommandBufferBeginInfo cmdBufferBeginInfo{};
  cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(sCmd.buffersTransfer[cmdBufferId], &cmdBufferBeginInfo);

  vkCmdCopyBuffer(sCmd.buffersTransfer[cmdBufferId], srcBuffer,
                  dstBuffer, 1, copyRegion);

  vkEndCommandBuffer(sCmd.buffersTransfer[cmdBufferId]);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &sCmd.buffersTransfer[cmdBufferId];

  vkQueueSubmit(logicalDevice.queues.transfer, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(logicalDevice.queues.transfer);

  return RE_OK;
}

TResult MGraphics::copyBuffer(RBuffer* srcBuffer, RBuffer* dstBuffer,
                              VkBufferCopy* copyRegion, uint32_t cmdBufferId) {
  if (cmdBufferId > MAX_TRANSFER_BUFFERS) {
    RE_LOG(Warning, "Invalid index of transfer buffer, using default.");
    cmdBufferId = 0;
  }

  VkCommandBufferBeginInfo cmdBufferBeginInfo{};
  cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(sCmd.buffersTransfer[cmdBufferId], &cmdBufferBeginInfo);

  vkCmdCopyBuffer(sCmd.buffersTransfer[cmdBufferId], srcBuffer->buffer,
                  dstBuffer->buffer, 1, copyRegion);

  vkEndCommandBuffer(sCmd.buffersTransfer[cmdBufferId]);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &sCmd.buffersTransfer[cmdBufferId];

  vkQueueSubmit(logicalDevice.queues.transfer, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(logicalDevice.queues.transfer);

  return RE_OK;
}

void MGraphics::updateCameraProjection(bool bUpdateAspectRatio, float FOV,
                                       float nearZ, float farZ) {
  if (bUpdateAspectRatio)
    sRender.sProjection.aspectRatio =
        (float)sSwapchain.imageExtent.width / sSwapchain.imageExtent.height;

  if (FOV > 0.0f) sRender.sProjection.FOV = FOV;
  if (nearZ > 0.0f) sRender.sProjection.nearZ = nearZ;
  if (farZ > 0.0f) sRender.sProjection.farZ = farZ;

  sRender.uboMVP.projection =
      glm::perspective(sRender.sProjection.FOV, sRender.sProjection.aspectRatio,
                       sRender.sProjection.nearZ, sRender.sProjection.farZ);
}
