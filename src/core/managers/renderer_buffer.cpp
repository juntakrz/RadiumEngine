#include "pch.h"
#include "core/core.h"
#include "core/material/texture.h"
#include "core/model/model.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::createBuffer(EBufferType type, VkDeviceSize size, RBuffer& outBuffer, void* inData)
{
  outBuffer.type = type;
  RBuffer stagingBuffer;
  VmaAllocationCreateInfo allocInfo{};
  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

  VkBufferDeviceAddressInfo bdaInfo{};
  bdaInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;

  std::vector<uint32_t> queueFamilyIndices = {
    (uint32_t)physicalDevice.queueFamilyIndices.graphics.at(0),
    (uint32_t)physicalDevice.queueFamilyIndices.transfer.at(0)
  };

  switch ((uint8_t)type) {
  case (uint8_t)EBufferType::STAGING: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

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
      outBuffer.allocInfo.pUserData = pData;
    }

    return RE_OK;
  }
  case (uint8_t)EBufferType::CPU_UNIFORM: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                      | VMA_ALLOCATION_CREATE_MAPPED_BIT;

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

  case (uint8_t)EBufferType::CPU_VERTEX: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                             | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                      | VMA_ALLOCATION_CREATE_MAPPED_BIT;

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

  case (uint8_t)EBufferType::CPU_INDEX: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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

  case (uint8_t)EBufferType::CPU_STORAGE: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                             | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                             | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo,
      &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create CPU_STORAGE buffer.");
      return RE_ERROR;
    };

    if (inData) {
      memcpy(outBuffer.allocInfo.pMappedData, inData, size);
    }

    bdaInfo.buffer = outBuffer.buffer;
    outBuffer.deviceAddress = vkGetBufferDeviceAddress(logicalDevice.device, &bdaInfo);

    return RE_OK;
  }

  case (uint8_t)EBufferType::DGPU_VERTEX: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.flags = NULL;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo,
      &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
      return RE_ERROR;
    };

    if (inData) {
      VkBufferCopy copyInfo{};
      copyInfo.srcOffset = 0;
      copyInfo.dstOffset = 0;
      copyInfo.size = size;

      createBuffer(EBufferType::STAGING, size, stagingBuffer, inData);
      copyBuffer(stagingBuffer.buffer, outBuffer.buffer, &copyInfo);
      vmaDestroyBuffer(memAlloc, stagingBuffer.buffer, stagingBuffer.allocation);
    }

    return RE_OK;
  }

  case (uint8_t)EBufferType::DGPU_INDEX: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.flags = NULL;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo, &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
      return RE_ERROR;
    };

    if (inData) {
      VkBufferCopy copyInfo{};
      copyInfo.srcOffset = 0;
      copyInfo.dstOffset = 0;
      copyInfo.size = size;

      createBuffer(EBufferType::STAGING, size, stagingBuffer, inData);
      copyBuffer(stagingBuffer.buffer, outBuffer.buffer, &copyInfo);
      vmaDestroyBuffer(memAlloc, stagingBuffer.buffer, stagingBuffer.allocation);
    }

    return RE_OK;
  }
  case (uint8_t)EBufferType::DGPU_STORAGE: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.flags = NULL;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo,
      &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
      return RE_ERROR;
    };

    if (inData) {
      VkBufferCopy copyInfo{};
      copyInfo.srcOffset = 0;
      copyInfo.dstOffset = 0;
      copyInfo.size = size;

      createBuffer(EBufferType::STAGING, size, stagingBuffer, inData);
      copyBuffer(stagingBuffer.buffer, outBuffer.buffer, &copyInfo);
      vmaDestroyBuffer(memAlloc, stagingBuffer.buffer, stagingBuffer.allocation);
    }

    bdaInfo.buffer = outBuffer.buffer;
    outBuffer.deviceAddress = vkGetBufferDeviceAddress(logicalDevice.device, &bdaInfo);

    return RE_OK;
  }
  case (uint8_t)EBufferType::DGPU_SAMPLER: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
      | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.flags = NULL;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo,
      &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
      return RE_ERROR;
    };

    if (inData) {
      VkBufferCopy copyInfo{};
      copyInfo.srcOffset = 0;
      copyInfo.dstOffset = 0;
      copyInfo.size = size;

      createBuffer(EBufferType::STAGING, size, stagingBuffer, inData);
      copyBuffer(stagingBuffer.buffer, outBuffer.buffer, &copyInfo);
      vmaDestroyBuffer(memAlloc, stagingBuffer.buffer, stagingBuffer.allocation);
    }

    bdaInfo.buffer = outBuffer.buffer;
    outBuffer.deviceAddress = vkGetBufferDeviceAddress(logicalDevice.device, &bdaInfo);

    return RE_OK;
  }
  case (uint8_t)EBufferType::DGPU_RESOURCE: {
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
      | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
      static_cast<uint32_t>(queueFamilyIndices.size());

    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.flags = NULL;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo,
      &outBuffer.buffer, &outBuffer.allocation,
      &outBuffer.allocInfo) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to create DGPU_VERTEX buffer.");
      return RE_ERROR;
    };

    if (inData) {
      VkBufferCopy copyInfo{};
      copyInfo.srcOffset = 0;
      copyInfo.dstOffset = 0;
      copyInfo.size = size;

      createBuffer(EBufferType::STAGING, size, stagingBuffer, inData);
      copyBuffer(stagingBuffer.buffer, outBuffer.buffer, &copyInfo);
      vmaDestroyBuffer(memAlloc, stagingBuffer.buffer, stagingBuffer.allocation);
    }

    bdaInfo.buffer = outBuffer.buffer;
    outBuffer.deviceAddress = vkGetBufferDeviceAddress(logicalDevice.device, &bdaInfo);

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
  VkBufferCopy* copyRegion,
  uint32_t cmdBufferId) {
  return copyBuffer(srcBuffer->buffer, dstBuffer->buffer, copyRegion,
    cmdBufferId);
}

TResult core::MRenderer::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage,
  uint32_t width, uint32_t height,
  uint32_t layerCount) {
  if (!srcBuffer || !dstImage) {
    RE_LOG(Error, "copyBufferToImage received nullptr as an argument.");
    return RE_ERROR;
  }

  VkCommandBuffer cmdBuffer = createCommandBuffer(
    ECmdType::Transfer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  VkBufferImageCopy imageCopy{};
  imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopy.imageSubresource.mipLevel = 0;
  imageCopy.imageSubresource.baseArrayLayer = 0;
  imageCopy.imageSubresource.layerCount = layerCount;
  imageCopy.imageExtent = { width, height, 1 };
  imageCopy.imageOffset = { 0, 0, 0 };
  imageCopy.bufferOffset = 0;
  imageCopy.bufferRowLength = 0;
  imageCopy.bufferImageHeight = 0;

  vkCmdCopyBufferToImage(cmdBuffer, srcBuffer, dstImage,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

  flushCommandBuffer(cmdBuffer, ECmdType::Transfer);

  return RE_OK;
}

TResult core::MRenderer::copyImageToBuffer(VkCommandBuffer commandBuffer, RTexture* pSrcImage, VkBuffer dstBuffer,
                                           uint32_t width, uint32_t height, VkImageSubresourceLayers* subresource) {
  if (!pSrcImage || !dstBuffer) {
    RE_LOG(Error, "copyImageToBuffer received nullptr as an argument.");
    return RE_ERROR;
  }

  VkBufferImageCopy imageCopy{};
  imageCopy.imageSubresource = *subresource;
  imageCopy.imageExtent = { width, height, 1 };
  imageCopy.imageOffset = { 0, 0, 0 };
  imageCopy.bufferOffset = 0;
  imageCopy.bufferRowLength = 0;
  imageCopy.bufferImageHeight = 0;

  vkCmdCopyImageToBuffer(commandBuffer, pSrcImage->texture.image, pSrcImage->texture.imageLayout, dstBuffer, 1, &imageCopy);

  return RE_OK;
}

// Runs in a dedicated thread
void core::MRenderer::updateInstanceBuffer() {
  std::vector<RInstanceData> instanceData(scene.totalInstances);

  uint32_t index = 0u;
  for (auto& model : scene.pModelReferences) {
    for (auto& primitive : model->m_pLinearPrimitives) {
      for (auto& instanceDataEntry : primitive->instanceData) {
        if (instanceDataEntry.isVisible) {
          instanceData[index] = instanceDataEntry.instanceBufferBlock;
          index++;
        }
      }
    }
  }

  memcpy(scene.instanceBuffers[renderView.frameInFlight].allocInfo.pMappedData,
         instanceData.data(), sizeof(RInstanceData) * instanceData.size());
}
