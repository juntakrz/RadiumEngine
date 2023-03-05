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
                                    VkBufferCopy* copyRegion,
                                    uint32_t cmdBufferId) {
  return copyBuffer(srcBuffer->buffer, dstBuffer->buffer, copyRegion,
                    cmdBufferId);
}

TResult core::MRenderer::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage,
                                           uint32_t width, uint32_t height) {
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
  imageCopy.imageSubresource.layerCount = 1;
  imageCopy.imageExtent = {width, height, 1};
  imageCopy.imageOffset = {0, 0, 0};
  imageCopy.bufferOffset = 0;
  imageCopy.bufferRowLength = 0;
  imageCopy.bufferImageHeight = 0;

  vkCmdCopyBufferToImage(cmdBuffer, srcBuffer, dstImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

  flushCommandBuffer(cmdBuffer, ECmdType::Transfer);

  return RE_OK;
}

VkCommandPool core::MRenderer::getCommandPool(ECmdType type) {
  switch (type) {
    case ECmdType::Graphics:
    return command.poolGraphics;
    case ECmdType::Compute:
    return command.poolCompute;
    case ECmdType::Transfer:
    return command.poolTransfer;
    default:
    return nullptr;
  }
}

VkQueue core::MRenderer::getCommandQueue(ECmdType type) {
  switch (type) {
    case ECmdType::Graphics:
    return logicalDevice.queues.graphics;
    case ECmdType::Compute:
    return logicalDevice.queues.compute;
    case ECmdType::Transfer:
    return logicalDevice.queues.transfer;
    default:
    return nullptr;
  }
}

VkCommandBuffer core::MRenderer::createCommandBuffer(ECmdType type,
                                                     VkCommandBufferLevel level,
                                                     bool begin) {
  VkCommandBuffer newCommandBuffer;
  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = getCommandPool(type);
  allocateInfo.commandBufferCount = 1;
  allocateInfo.level = level;

  vkAllocateCommandBuffers(logicalDevice.device, &allocateInfo,
                           &newCommandBuffer);

  if (begin) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(newCommandBuffer, &beginInfo);
  }

  return newCommandBuffer;
}

void core::MRenderer::beginCommandBuffer(VkCommandBuffer cmdBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(cmdBuffer, &beginInfo);
}

void core::MRenderer::flushCommandBuffer(VkCommandBuffer cmdBuffer,
                                         ECmdType type, bool free,
                                         bool useFence) {
  vkEndCommandBuffer(cmdBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pCommandBuffers = &cmdBuffer;
  submitInfo.commandBufferCount = 1;

  VkQueue cmdQueue = getCommandQueue(type);
  VkFence fence = VK_NULL_HANDLE;

  if (useFence) {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = NULL;  // unsignaled
    vkCreateFence(logicalDevice.device, &fenceInfo, nullptr, &fence);
  }

  vkQueueSubmit(cmdQueue, 1, &submitInfo, fence);

  switch (useFence) {
    case false: {
    vkQueueWaitIdle(cmdQueue);
    break;
    }
    case true: {
    // fence timeout is 1 second
    vkWaitForFences(logicalDevice.device, 1, &fence, VK_TRUE, 1000000000uLL);
    break;
    }
  }

  if (free) {
    vkFreeCommandBuffers(logicalDevice.device, getCommandPool(type), 1,
                         &cmdBuffer);
  }
}

VkImageView core::MRenderer::createImageView(VkImage image, VkFormat format,
                                             uint32_t levelCount,
                                             uint32_t layerCount) {
  VkImageView imageView = nullptr;

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;

  viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = levelCount;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = layerCount;

  if (vkCreateImageView(logicalDevice.device, &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    RE_LOG(Error, "failed to create image view with format id %d.", format);

    return nullptr;
  }

  return imageView;
}

uint32_t core::MRenderer::bindPrimitive(WPrimitive* pPrimitive) {
  if (!pPrimitive) {
    RE_LOG(Error, "No primitive provided for binding.");
    return -1;
  }

  system.meshes.emplace_back(pPrimitive);
  return (uint32_t)system.meshes.size() - 1;
}

void core::MRenderer::bindPrimitive(
    const std::vector<WPrimitive*>& inPrimitives,
    std::vector<uint32_t>& outIndices) {
  outIndices.clear();

  for (const auto& it : inPrimitives) {
    system.meshes.emplace_back(it);
    outIndices.emplace_back(static_cast<uint32_t>(system.meshes.size() - 1));
  }
}

void core::MRenderer::unbindPrimitive(uint32_t index) {
  if (index > system.meshes.size() - 1) {
    RE_LOG(Error, "Failed to unbind primitive at %d. Index is out of bounds.",
           index);
    return;
  }

#ifndef NDEBUG
  if (system.meshes[index] == nullptr) {
    RE_LOG(Warning, "Failed to unbind primitive at %d. It's already unbound.",
           index);
    return;
  }
#endif

  system.meshes[index] = nullptr;
}

void core::MRenderer::unbindPrimitive(
    const std::vector<uint32_t>& meshIndices) {
  uint32_t bindsNum = static_cast<uint32_t>(system.meshes.size());

  for (const auto& index : meshIndices) {
    if (index > bindsNum - 1) {
    RE_LOG(Error, "Failed to unbind primitive at %d. Index is out of bounds.",
           index);
    return;
    }

#ifndef NDEBUG
    if (system.meshes[index] == nullptr) {
    RE_LOG(Warning, "Failed to unbind primitive at %d. It's already unbound.",
           index);
    return;
    }
#endif

    system.meshes[index] = nullptr;
  }
}

void core::MRenderer::clearPrimitiveBinds() { system.meshes.clear(); }

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