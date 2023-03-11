#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/ref.h"
#include "core/managers/renderer.h"
#include "core/managers/actors.h"
#include "core/world/model/model.h"
#include "core/world/actors/camera.h"

// PRIVATE

TResult core::MRenderer::setDepthStencilFormat() {
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT
  };
  VkBool32 validDepthFormat = false;
  for (auto& format : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physicalDevice.device, format, &formatProps);
    if (formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      images.depth.format = format;
      validDepthFormat = true;
      break;
    }
  }

  if (!validDepthFormat) {
    RE_LOG(Critical, "Failed to find an appropriate depth/stencil format.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

// PUBLIC

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
    // staging buffer (won't be allocated if no data for it is provided)
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc{};
    VmaAllocationInfo stagingAllocInfo{};

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

    std::vector<uint32_t> queueFamilyIndices = {
        (uint32_t)physicalDevice.queueFamilyIndices.graphics.at(0),
        (uint32_t)physicalDevice.queueFamilyIndices.transfer.at(0)};

    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    bufferCreateInfo.queueFamilyIndexCount =
        static_cast<uint32_t>(queueFamilyIndices.size());

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (inData) {
      if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo,
                          &stagingBuffer, &stagingAlloc,
                          &stagingAllocInfo) != VK_SUCCESS) {
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
    }

    // destination buffer
    bufferCreateInfo.usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

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

      copyBuffer(stagingBuffer, outBuffer.buffer, &copyInfo);

      vmaDestroyBuffer(memAlloc, stagingBuffer, stagingAlloc);
    }

    return RE_OK;
  }

  case (uint8_t)EBufferMode::DGPU_INDEX: {

    // staging buffer (won't be allocated if no data for it is provided)
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
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

    if (inData) {
      if (vmaCreateBuffer(memAlloc, &bufferCreateInfo, &allocInfo,
                          &stagingBuffer, &stagingAlloc,
                          &stagingAllocInfo) != VK_SUCCESS) {
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
    }

    // destination vertex buffer
    bufferCreateInfo.usage =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

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

      copyBuffer(stagingBuffer, outBuffer.buffer, &copyInfo);

      vmaDestroyBuffer(memAlloc, stagingBuffer, stagingAlloc);
    }

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

void core::MRenderer::transitionImageLayout(VkImage image, VkFormat format,
                                             VkImageLayout oldLayout,
                                             VkImageLayout newLayout, ECmdType cmdType) {
  VkCommandBuffer cmdBuffer =
      createCommandBuffer(cmdType, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  VkImageMemoryBarrier imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageBarrier.image = image;
  imageBarrier.oldLayout = oldLayout;
  imageBarrier.newLayout = newLayout;
  imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.levelCount = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = 1;

  // change aspect mask if image is of a depth/stencil type
  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    imageBarrier.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  VkPipelineStageFlags srcStageFlags;
  VkPipelineStageFlags dstStageFlags;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    imageBarrier.srcAccessMask = NULL;
    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    imageBarrier.srcAccessMask = 0;
    imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    RE_LOG(Error, "Unsupported layout transition.");
    return;
  }

  vkCmdPipelineBarrier(cmdBuffer, srcStageFlags, dstStageFlags, NULL, NULL,
                       nullptr, NULL, nullptr, 1, &imageBarrier);

  core::renderer.flushCommandBuffer(cmdBuffer, cmdType, true);
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

  if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
      format == VK_FORMAT_D24_UNORM_S8_UINT) {
    viewInfo.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  if (vkCreateImageView(logicalDevice.device, &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    RE_LOG(Error, "failed to create image view with format id %d.", format);

    return nullptr;
  }

  return imageView;
}

uint32_t core::MRenderer::bindModel(WModel* pModel) {
  if (!pModel) {
    RE_LOG(Error, "No model provided for binding.");
    return -1;
  }

  // check if model is already bound, it shouldn't have valid offsets stored
  if (pModel->m_isBound) {
    RE_LOG(Error, "Model is already bound.");
    return -1;
  }

  // copy vertex buffer
  VkBufferCopy copyInfo{};
  copyInfo.srcOffset = 0u;
  copyInfo.dstOffset = scene.currentVertexOffset;
  copyInfo.size = sizeof(RVertex) * pModel->m_vertexCount;

  copyBuffer(&pModel->staging.vertexBuffer, &scene.vertexBuffer, &copyInfo);

  // copy index buffer
  copyInfo.dstOffset = scene.currentIndexOffset;
  copyInfo.size = sizeof(uint32_t) * pModel->m_indexCount;

  copyBuffer(&pModel->staging.indexBuffer, &scene.indexBuffer, &copyInfo);

  // add model to rendering queue, store its offsets
  RModelBindInfo bindInfo{};
  bindInfo.pModel = pModel;
  bindInfo.vertexOffset = scene.currentVertexOffset;
  bindInfo.vertexCount = pModel->m_vertexCount;
  bindInfo.indexOffset = scene.currentIndexOffset;
  bindInfo.indexCount = pModel->m_indexCount;

  system.models.emplace_back(bindInfo);

  pModel->m_isBound = true;

  // store new offsets into scene buffer data
  scene.currentVertexOffset += pModel->m_vertexCount;
  scene.currentIndexOffset += pModel->m_indexCount;

  pModel->clearStagingData();

#ifndef NDEBUG
  RE_LOG(Log, "Bound model \"%s\" to graphics pipeline.", pModel->getName());
#endif

  return (uint32_t)system.models.size() - 1;
}

void core::MRenderer::bindModel(const std::vector<WModel*>& inModels,
                                std::vector<uint32_t>& outIndices) {
  outIndices.clear();

  for (const auto& it : inModels) {
    system.models.emplace_back(it);
    outIndices.emplace_back(static_cast<uint32_t>(system.models.size() - 1));

#ifndef NDEBUG
    RE_LOG(Log, "Bound model \"%s\" to graphics pipeline.", it->getName());
#endif
  }
}

void core::MRenderer::unbindModel(uint32_t index) {
  if (index > system.models.size() - 1) {
    RE_LOG(Error, "Failed to unbind model at %d. Index is out of bounds.",
           index);
    return;
  }

#ifndef NDEBUG
  if (system.models[index].pModel == nullptr) {
    RE_LOG(Warning, "Failed to unbind model at %d. It's already unbound.",
           index);
    return;
  }
#endif

  system.models[index].pModel = nullptr;
}

void core::MRenderer::unbindModel(
    const std::vector<uint32_t>& modelIndices) {
  uint32_t bindsNum = static_cast<uint32_t>(system.models.size());

  for (const auto& index : modelIndices) {
    if (index > bindsNum - 1) {
    RE_LOG(Error, "Failed to unbind model at %d. Index is out of bounds.",
           index);
    return;
    }

#ifndef NDEBUG
    if (system.models[index].pModel == nullptr) {
    RE_LOG(Warning, "Failed to unbind model at %d. It's already unbound.",
           index);
    return;
    }
#endif

    system.models[index].pModel = nullptr;
  }
}

void core::MRenderer::clearModelBinds() { system.models.clear(); }

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