#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/ref.h"
#include "core/managers/renderer.h"
#include "core/managers/actors.h"
#include "core/model/model.h"
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
      core::vulkan::formatDepth = format;
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


VkPipelineShaderStageCreateInfo core::MRenderer::loadShader(
    const char* path, VkShaderStageFlagBits stage) {
  std::string fullPath = RE_PATH_SHADERS + std::string(path);
  std::vector<char> shaderCode = util::readFile(fullPath.c_str());

  VkPipelineShaderStageCreateInfo stageCreateInfo{};
  stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageCreateInfo.stage = stage;
  stageCreateInfo.module = createShaderModule(shaderCode);
  stageCreateInfo.pName = "main";

  return stageCreateInfo;
}

VkShaderModule core::MRenderer::createShaderModule(
    std::vector<char>& shaderCode) {
  VkShaderModuleCreateInfo smInfo{};
  smInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  smInfo.codeSize = shaderCode.size();
  smInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());

  VkShaderModule shaderModule;
  if ((vkCreateShaderModule(logicalDevice.device, &smInfo, nullptr,
                            &shaderModule) != VK_SUCCESS)) {
    RE_LOG(Warning, "failed to create requested shader module.");
    return VK_NULL_HANDLE;
  };

  return shaderModule;
}

TResult core::MRenderer::checkInstanceValidationLayers() {
  uint32_t layerCount = 0;
  std::vector<VkLayerProperties> availableValidationLayers;
  VkResult checkResult;
  bool bRequestedLayersAvailable = false;
  std::string errorLayer;

  // enumerate instance layers
  checkResult = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  availableValidationLayers.resize(layerCount);
  checkResult = vkEnumerateInstanceLayerProperties(
      &layerCount, availableValidationLayers.data());
  if (checkResult != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to enumerate instance layer properties.");
    return RE_CRITICAL;
  }

  // check if all requested validation layers are available
  for (const char* requestedLayer : debug::validationLayers) {
    bRequestedLayersAvailable = false;
    for (const auto& availableLayer : availableValidationLayers) {
      if (strcmp(requestedLayer, availableLayer.layerName) == 0) {
        bRequestedLayersAvailable = true;
        break;
      }
    }
    if (!bRequestedLayersAvailable) {
      errorLayer = std::string(requestedLayer);
      break;
    }
  }

  if (!bRequestedLayersAvailable) {
    RE_LOG(Critical,
           "Failed to detect all requested validation layers. Layer '%s' not "
           "present.",
           errorLayer.c_str());
    return RE_CRITICAL;
  }

  return RE_OK;
}

std::vector<const char*> core::MRenderer::getRequiredInstanceExtensions() {
  uint32_t extensionCount = 0;
  const char** ppExtensions;

  ppExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
  std::vector<const char*> requiredExtensions(ppExtensions,
                                              ppExtensions + extensionCount);

  if (bRequireValidationLayers) {
    requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return requiredExtensions;
}

std::vector<VkExtensionProperties> core::MRenderer::getInstanceExtensions() {
  uint32_t extensionCount = 0;
  std::vector<VkExtensionProperties> extensionProperties;

  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  extensionProperties.resize(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensionProperties.data());

  return extensionProperties;
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
      outBuffer.allocInfo.pUserData = pData;
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

void core::MRenderer::setImageLayout(VkCommandBuffer cmdBuffer,
                                     RTexture* pTexture,
                                     VkImageLayout newLayout,
                                     VkImageSubresourceRange subresourceRange) {
  setImageLayout(cmdBuffer, pTexture->texture.image,
                 pTexture->texture.imageLayout, newLayout, subresourceRange);
  
  pTexture->texture.imageLayout = newLayout;
  pTexture->texture.descriptor.imageLayout = newLayout;
}

void core::MRenderer::setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                                     VkImageLayout oldLayout,
                                     VkImageLayout newLayout,
                                     VkImageSubresourceRange subresourceRange) {
  if (newLayout == oldLayout) {
#ifndef NDEBUG
    RE_LOG(Warning, "Trying to convert texture to the same image layout.");
#endif
    return;
  }

  // Create an image barrier object
  VkImageMemoryBarrier imageMemoryBarrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = NULL,
      // Some default values
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED};

  imageMemoryBarrier.oldLayout = oldLayout;
  imageMemoryBarrier.newLayout = newLayout;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange = subresourceRange;

  // Source layouts (old)
  // The source access mask controls actions to be finished on the old
  // layout before it will be transitioned to the new layout.
  switch (oldLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
    // Image layout is undefined (or does not matter).
    // Only valid as initial layout. No flags required.
    imageMemoryBarrier.srcAccessMask = 0;
    break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
    // Image is preinitialized.
    // Only valid as initial layout for linear images; preserves memory
    // contents. Make sure host writes have finished.
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image is a color attachment.
    // Make sure writes to the color buffer have finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image is a depth/stencil attachment.
    // Make sure any writes to the depth/stencil buffer have finished.
    imageMemoryBarrier.srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image is a transfer source.
    // Make sure any reads from the image have finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image is a transfer destination.
    // Make sure any writes to the image have finished.
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image is read by a shader.
    // Make sure any shader reads from the image have finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;

    default:
    /* Value not used by callers, so not supported. */
    assert(KTX_FALSE);
  }

  // Target layouts (new)
  // The destination access mask controls the dependency for the new image
  // layout.
  switch (newLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image will be used as a transfer destination.
    // Make sure any writes to the image have finished.
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image will be used as a transfer source.
    // Make sure any reads from and writes to the image have finished.
    imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image will be used as a color attachment.
    // Make sure any writes to the color buffer have finished.
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image layout will be used as a depth/stencil attachment.
    // Make sure any writes to depth/stencil buffer have finished.
    imageMemoryBarrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image will be read in a shader (sampler, input attachment).
    // Make sure any writes to the image have finished.
    if (imageMemoryBarrier.srcAccessMask == 0) {
      imageMemoryBarrier.srcAccessMask =
          VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
    default:
    /* Value not used by callers, so not supported. */
    assert(KTX_FALSE);
  }

  // Put barrier on top of pipeline.
  VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  VkPipelineStageFlags dstStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  // Add the barrier to the passed command buffer
  vkCmdPipelineBarrier(cmdBuffer, srcStageFlags, dstStageFlags, 0, 0,
                               NULL, 0, NULL, 1, &imageMemoryBarrier);
}

TResult core::MRenderer::generateMipMaps(RTexture* pTexture, int32_t mipLevels,
                                         VkFilter filter) {
  if (!pTexture) {
    RE_LOG(Error, "Can't generate mip maps, no texture was provided.");
    return RE_ERROR;
  }

  if (mipLevels < 1) {
    RE_LOG(Warning, "Tried to generate 0 mip maps for texture '%s'. Skipping.",
           pTexture->name);
    return RE_WARNING;
  }

  int32_t mipWidth = 0;
  int32_t mipHeight = 0;

  VkImageSubresourceRange range{};
  range.baseArrayLayer = 0;
  range.layerCount = pTexture->texture.layerCount;
  range.baseMipLevel = 0;
  range.levelCount = pTexture->texture.levelCount;

  if (pTexture->texture.imageFormat != VK_FORMAT_D32_SFLOAT_S8_UINT &&
      pTexture->texture.imageFormat != VK_FORMAT_D24_UNORM_S8_UINT) {
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  } else {
    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  VkCommandBuffer cmdBuffer = createCommandBuffer(
      ECmdType::Graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // transition texture to DST layout if needed
  if (pTexture->texture.imageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    setImageLayout(cmdBuffer, pTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   range);
  }

  // Create an image barrier object
  VkImageMemoryBarrier imageMemoryBarrier{};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.pNext = NULL;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.image = pTexture->texture.image;
  imageMemoryBarrier.subresourceRange.aspectMask = range.aspectMask;
  imageMemoryBarrier.subresourceRange.layerCount = 1;
  imageMemoryBarrier.subresourceRange.levelCount = 1;

  VkImageBlit blit{};
  blit.srcOffsets[0] = {0, 0, 0};
  blit.srcSubresource.layerCount = 1;
  blit.dstOffsets[0] = {0, 0, 0};
  blit.dstSubresource.layerCount = 1;

  for (uint32_t j = 0; j < range.layerCount; ++j) {
    mipWidth = pTexture->texture.width;
    mipHeight = pTexture->texture.height;

    imageMemoryBarrier.subresourceRange.baseArrayLayer = j;


    blit.srcSubresource.aspectMask = range.aspectMask;
    blit.srcSubresource.baseArrayLayer = j;

    blit.dstSubresource.aspectMask = range.aspectMask;
    blit.dstSubresource.baseArrayLayer = j;

    for (int32_t i = 1; i < mipLevels; ++i) {
      // creating a new mipmap using the previous mip level
      imageMemoryBarrier.subresourceRange.baseMipLevel = i - 1;
      imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                           nullptr, 1, &imageMemoryBarrier);

      blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
      blit.srcSubresource.mipLevel = i - 1;
      blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                            mipHeight > 1 ? mipHeight / 2 : 1, 1};
      blit.dstSubresource.mipLevel = i;

      vkCmdBlitImage(cmdBuffer, pTexture->texture.image,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     pTexture->texture.image,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);

      if (mipWidth > 1) mipWidth /= 2;
      if (mipHeight > 1) mipHeight /= 2;
      
      imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                           0, nullptr, 1, &imageMemoryBarrier);
    }

    imageMemoryBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &imageMemoryBarrier);
  }

  flushCommandBuffer(cmdBuffer, ECmdType::Graphics, true);

  pTexture->texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  return RE_OK;
}


TResult core::MRenderer::generateSingleMipMap(VkCommandBuffer cmdBuffer,
                                              RTexture* pTexture,
                                              uint32_t mipLevel, uint32_t layer,
                                              VkFilter filter) {
  if (!pTexture) {
    RE_LOG(Error, "Can't generate mip maps, no texture was provided.");
    return RE_ERROR;
  }

  if (mipLevel < 1 || mipLevel > pTexture->texture.levelCount - 1 ||
      layer < 0 || layer > pTexture->texture.layerCount - 1) {
    RE_LOG(Error,
           "Invalid mip map generation parameters for texture '%s'. Skipping.",
           pTexture->name.c_str());
    return RE_ERROR;
  }

  VkPipelineStageFlags srcStageMask = 0;

  switch (pTexture->texture.descriptor.imageLayout) {
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
      srcStageMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    }
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
      srcStageMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    }
    default: {
      RE_LOG(Error,
             "Unsupported layout for mip map generation for texture '%s'. "
             "Skipping.",
             pTexture->name.c_str());
      return RE_ERROR;
    }
  }

  const int32_t mipWidth = pTexture->texture.width / (1 << mipLevel - 1);
  const int32_t mipHeight = pTexture->texture.height / (1 << mipLevel - 1);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = NULL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = pTexture->texture.image;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = layer;
  barrier.subresourceRange.baseMipLevel = mipLevel - 1;
  barrier.oldLayout = pTexture->texture.imageLayout;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

  switch (pTexture->texture.imageLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    }
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
      barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    }
    default: {
      RE_LOG(
          Error,
          "Invalid input texture layout when trying to generate a mip map for "
          "'%s'. Layout must be either VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or "
          "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.",
          pTexture->name.c_str());

      return RE_ERROR;
    }
  }

  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  if (pTexture->texture.imageFormat != VK_FORMAT_D32_SFLOAT_S8_UINT &&
      pTexture->texture.imageFormat != VK_FORMAT_D24_UNORM_S8_UINT) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  } else {
    barrier.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  // convert source mip level to TRANSFER SRC layout
  vkCmdPipelineBarrier(cmdBuffer, srcStageMask,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  // convert destination mip level to TRANSFER DST layout if differs
  if (pTexture->texture.imageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.oldLayout = pTexture->texture.imageLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuffer, srcStageMask,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);
  }

  VkImageBlit blit{};
  blit.srcOffsets[0] = {0, 0, 0};
  blit.srcSubresource.layerCount = 1;
  blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
  blit.srcSubresource.aspectMask = barrier.subresourceRange.aspectMask;
  blit.srcSubresource.baseArrayLayer = layer;
  blit.srcSubresource.mipLevel = mipLevel - 1;
  blit.dstOffsets[0] = {0, 0, 0};
  blit.dstSubresource.layerCount = 1;
  blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                        mipHeight > 1 ? mipHeight / 2 : 1, 1};
  blit.dstSubresource.aspectMask = barrier.subresourceRange.aspectMask;
  blit.dstSubresource.baseArrayLayer = layer;
  blit.dstSubresource.mipLevel = mipLevel;

  vkCmdBlitImage(cmdBuffer, pTexture->texture.image,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pTexture->texture.image,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);

  // convert source mip map to its original layout
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.newLayout = pTexture->texture.imageLayout;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.subresourceRange.baseMipLevel = mipLevel - 1;

  switch (pTexture->texture.imageLayout) {
    // leave mip map at 'mipLevel' as is because it already has a valid layout
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    }
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                           0, nullptr, 1, &barrier);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.subresourceRange.baseMipLevel = mipLevel;

      break;
    }
  }

  vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

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
    vkDestroyFence(logicalDevice.device, fence, nullptr);
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
  viewInfo.format = format;

  layerCount == 6 ? viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE
                  : viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

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

uint32_t core::MRenderer::bindEntity(AEntity* pEntity) {
  if (!pEntity) {
    RE_LOG(Error, "No world entity was provided for binding.");
    return -1;
  }

  // check if model is already bound, it shouldn't have valid offsets stored
  if (pEntity->getBindingIndex() > -1) {
    RE_LOG(Error, "Entity is already bound.");
    return -1;
  }

  WModel* pModel = pEntity->getModel();

  // copy vertex buffer
  VkBufferCopy copyInfo{};
  copyInfo.srcOffset = 0u;
  copyInfo.dstOffset = scene.currentVertexOffset * sizeof(RVertex);
  copyInfo.size = sizeof(RVertex) * pModel->m_vertexCount;

  copyBuffer(&pModel->staging.vertexBuffer, &scene.vertexBuffer, &copyInfo);

  // copy index buffer
  copyInfo.dstOffset = scene.currentIndexOffset * sizeof(uint32_t);
  copyInfo.size = sizeof(uint32_t) * pModel->m_indexCount;

  copyBuffer(&pModel->staging.indexBuffer, &scene.indexBuffer, &copyInfo);

  // add model to rendering queue, store its offsets
  REntityBindInfo bindInfo{};
  bindInfo.pEntity = pEntity;
  bindInfo.vertexOffset = scene.currentVertexOffset;
  bindInfo.vertexCount = pModel->m_vertexCount;
  bindInfo.indexOffset = scene.currentIndexOffset;
  bindInfo.indexCount = pModel->m_indexCount;

  system.bindings.emplace_back(bindInfo);

  pEntity->setBindingIndex(static_cast<int32_t>(system.bindings.size() - 1));

  // store new offsets into scene buffer data
  scene.currentVertexOffset += pModel->m_vertexCount;
  scene.currentIndexOffset += pModel->m_indexCount;

  pModel->clearStagingData();

#ifndef NDEBUG
  RE_LOG(Log, "Bound model \"%s\" to graphics pipeline.", pModel->getName());
#endif

  return (uint32_t)system.bindings.size() - 1;
}

void core::MRenderer::unbindEntity(uint32_t index) {
  if (index > system.bindings.size() - 1) {
    RE_LOG(Error, "Failed to unbind entity at %d. Index is out of bounds.",
           index);
    return;
  }

#ifndef NDEBUG
  if (system.bindings[index].pEntity == nullptr) {
    RE_LOG(Warning, "Failed to unbind entity at %d. It's already unbound.",
           index);
    return;
  }
#endif

  system.bindings[index].pEntity->setBindingIndex(-1);
  system.bindings[index].pEntity = nullptr;
}

void core::MRenderer::clearBoundEntities() { system.bindings.clear(); }

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