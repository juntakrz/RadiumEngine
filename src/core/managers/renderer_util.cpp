#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/ref.h"
#include "core/managers/renderer.h"
#include "core/managers/actors.h"
#include "core/model/model.h"
#include "core/world/actors/camera.h"

// PRIVATE

VkRenderPass core::MRenderer::createRenderPass(
    VkDevice device, uint32_t colorAttachments,
    VkAttachmentDescription* pColorAttachments,
    VkAttachmentDescription* pDepthAttachment) {
  // we need to put all the color and the depth attachments in the same buffer
  //
  VkAttachmentDescription attachments[10];
  assert(colorAttachments <
         10);  // make sure we don't overflow the scratch buffer above

  memcpy(attachments, pColorAttachments,
         sizeof(VkAttachmentDescription) * colorAttachments);
  if (pDepthAttachment != NULL) {
    memcpy(&attachments[colorAttachments], pDepthAttachment,
           sizeof(VkAttachmentDescription));
  }

  // create references for the attachments
  //
  VkAttachmentReference colorReference[10];
  for (uint32_t i = 0; i < colorAttachments; i++)
    colorReference[i] = {i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentReference depthReference = {
      colorAttachments, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  // Create subpass
  //
  VkSubpassDescription subpassDesc{};
  subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDesc.flags = 0;
  subpassDesc.inputAttachmentCount = 0;
  subpassDesc.pInputAttachments = NULL;
  subpassDesc.colorAttachmentCount = colorAttachments;
  subpassDesc.pColorAttachments = colorReference;
  subpassDesc.pResolveAttachments = NULL;
  subpassDesc.pDepthStencilAttachment =
      (pDepthAttachment) ? &depthReference : NULL;
  subpassDesc.preserveAttachmentCount = 0;
  subpassDesc.pPreserveAttachments = NULL;

  VkSubpassDependency subpassDependency{};
  subpassDependency.dependencyFlags = 0;
  subpassDependency.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT |
      ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
      ((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
  subpassDependency.dstStageMask =
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
      ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
      ((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                          : 0);
  subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependency.srcAccessMask =
      ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
      ((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
  subpassDependency.srcStageMask =
      ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
      ((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                          : 0);
  subpassDependency.srcSubpass = 0;

  // Create render pass
  //
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pNext = NULL;
  renderPassInfo.attachmentCount = colorAttachments;
  if (pDepthAttachment != NULL) renderPassInfo.attachmentCount++;
  renderPassInfo.pAttachments = attachments;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDesc;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &subpassDependency;

  VkRenderPass renderPass;
  VkResult result =
      vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
  assert(result == VK_SUCCESS);

  /*setResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass,
                  "CreateRenderPass");*/

  return renderPass;
}

TResult core::MRenderer::createFramebuffer(
    ERenderPass renderPass, const std::vector<std::string>& attachmentNames,
    const char* framebufferName) {
#ifndef NDEBUG
  RE_LOG(Log, "Creating framebuffer '%s' with %d attachments.", framebufferName,
         attachmentNames.size());
#endif

  if (attachmentNames.empty()) {
    RE_LOG(Error,
           "Failed to create framebuffer '%s'. No texture attachments were "
           "provided.",
           framebufferName);

    return RE_ERROR;
  }

  std::vector<VkImageView> imageViews;
  uint32_t width = 0, height = 0, layerCount = 0;

  for (const auto& textureName : attachmentNames) {
    RTexture* fbTarget = core::resources.getTexture(textureName.c_str());

    if (!fbTarget || !fbTarget->texture.view) {
      RE_LOG(Error,
             "Failed to retrieve attachment texture '%s' for creating "
             "framebuffer '%s'.",
             textureName.c_str(), framebufferName);

      return RE_ERROR;
    }

    if (!imageViews.empty()) {
      if (fbTarget->texture.width != width ||
          fbTarget->texture.height != height ||
          fbTarget->texture.layerCount != layerCount) {
        RE_LOG(Error,
               "Failed to create framebuffer '%s'. Texture dimensions are not "
               "equal for all attachments.",
               framebufferName);

        return RE_ERROR;
      }
    } else {
      width = fbTarget->texture.width;
      height = fbTarget->texture.height;
      layerCount = fbTarget->texture.layerCount;
    }

    imageViews.emplace_back(fbTarget->texture.view);
  }

  std::string fbName = framebufferName;

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = getVkRenderPass(renderPass);
  framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
  framebufferInfo.pAttachments = imageViews.data();
  framebufferInfo.width = width;
  framebufferInfo.height = height;
  framebufferInfo.layers = layerCount;

  if (!system.framebuffers.try_emplace(fbName).second) {
#ifndef NDEBUG
    RE_LOG(Warning,
           "Failed to create framebuffer record for \"%s\". Already exists.",
           fbName.c_str());
#endif
    return RE_WARNING;
  }

  if (vkCreateFramebuffer(logicalDevice.device, &framebufferInfo, nullptr,
                          &system.framebuffers.at(fbName)) != VK_SUCCESS) {
    RE_LOG(Error, "failed to create framebuffer %s.", fbName.c_str());

    return RE_ERROR;
  }

  return RE_OK;
}

RTexture* core::MRenderer::createFragmentRenderTarget(const char* name) {
  RTextureInfo textureInfo{};
  textureInfo.name = name;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.width = swapchain.imageExtent.width;
  textureInfo.height = swapchain.imageExtent.height;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.usageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  RTexture* pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Error, "Failed to create fragment render target \"%s\".", name);
    return nullptr;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created fragment render target '%s'.", name);
#endif

  return pNewTexture;
}

void core::MRenderer::setResourceName(VkDevice device, VkObjectType objectType,
                                      uint64_t handle, const char* name) {
  /*if (s_vkSetDebugUtilsObjectName && handle && name) {
    std::unique_lock<std::mutex> lock(s_mutex);

    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = handle;
    nameInfo.pObjectName = name;
    s_vkSetDebugUtilsObjectName(device, &nameInfo);
  }*/
}

TResult core::MRenderer::getDepthStencilFormat(
    VkFormat desiredFormat, VkFormat& outFormat) {
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT
  };

  VkFormatProperties formatProps;
  vkGetPhysicalDeviceFormatProperties(physicalDevice.device, desiredFormat,
                                      &formatProps);
  if (formatProps.optimalTilingFeatures &
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
    outFormat = desiredFormat;
    return RE_OK;
  }

  RE_LOG(Warning,
         "Failed to set depth(stencil) format %d. Unsupported by the GPU. "
         "Trying to use the nearest compatible format.",
         desiredFormat);

  for (auto& format : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physicalDevice.device, format,
                                        &formatProps);
    if (formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      outFormat = format;
      return RE_WARNING;
    }
  }

  RE_LOG(Critical, "Failed to find an appropriate depth/stencil format.");
  return RE_CRITICAL;
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

TResult core::MRenderer::copyImage(VkCommandBuffer cmdBuffer, VkImage srcImage,
                                   VkImage dstImage,
                                   VkImageLayout srcImageLayout,
                                   VkImageLayout dstImageLayout,
                                   VkImageCopy& copyRegion) {
  VkImageSubresourceRange srcRange{};
  srcRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  srcRange.baseArrayLayer = 0;
  srcRange.layerCount = 1;
  srcRange.baseMipLevel = 0;
  srcRange.levelCount = 1;

  VkImageSubresourceRange dstRange = srcRange;

  setImageLayout(cmdBuffer, srcImage, srcImageLayout,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcRange);

  setImageLayout(cmdBuffer, dstImage, dstImageLayout,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstRange);

  vkCmdCopyImage(cmdBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                 &swapchain.copyRegion);

  setImageLayout(cmdBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 srcImageLayout, srcRange);

  setImageLayout(cmdBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 dstImageLayout, dstRange);

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

    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
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

void core::MRenderer::convertRenderTargets(VkCommandBuffer cmdBuffer,
                                           std::vector<RTexture*>* pInTextures,
                                           bool convertBackToRenderTargets) {
  VkImageLayout newLayout = (convertBackToRenderTargets)
                                ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkImageSubresourceRange range{};
  range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  range.baseArrayLayer = 0;
  range.baseMipLevel = 0;

  for (int8_t i = 0; i < pInTextures->size(); ++i) {
    if (newLayout == pInTextures->at(i)->texture.imageLayout) continue;

    range.layerCount = pInTextures->at(i)->texture.layerCount;
    range.levelCount = pInTextures->at(i)->texture.levelCount;

    setImageLayout(cmdBuffer, pInTextures->at(i), newLayout, range);
  }
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

  const int32_t mipWidth = pTexture->texture.width / (1 << (mipLevel - 1));
  const int32_t mipHeight = pTexture->texture.height / (1 << (mipLevel - 1));

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

  if (layerCount == 6) {
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  } else if (layerCount > 1) {
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  } else {
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  }

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
      format == VK_FORMAT_D24_UNORM_S8_UINT ||
      format == VK_FORMAT_D16_UNORM_S8_UINT) {
    viewInfo.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  } else if (format == VK_FORMAT_D32_SFLOAT) {
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
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
  if (pEntity->getRendererBindingIndex() > -1) {
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

  // store reference data to model in the scene buffers
  pModel->setSceneBindingData(scene.currentVertexOffset,
                              scene.currentIndexOffset);

  // add model to rendering queue, store its offsets
  REntityBindInfo bindInfo{};
  bindInfo.pEntity = pEntity;
  bindInfo.vertexOffset = scene.currentVertexOffset;
  bindInfo.vertexCount = pModel->m_vertexCount;
  bindInfo.indexOffset = scene.currentIndexOffset;
  bindInfo.indexCount = pModel->m_indexCount;

  system.bindings.emplace_back(bindInfo);

  // TODO: implement indirect draw command properly
  VkDrawIndexedIndirectCommand drawCommand{};
  drawCommand.firstInstance = 0;
  drawCommand.instanceCount = 1;
  drawCommand.vertexOffset = scene.currentVertexOffset;
  drawCommand.firstIndex = scene.currentIndexOffset;
  drawCommand.indexCount = pModel->m_indexCount;

  system.drawCommands.emplace_back(drawCommand);
  // TODO

  pEntity->setRendererBindingIndex(
      static_cast<int32_t>(system.bindings.size() - 1));

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

  system.bindings[index].pEntity->setRendererBindingIndex(-1);
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

void core::MRenderer::setSunCamera(const char* name) {
  if (ACamera* pCamera = core::ref.getActor(name)->getAs<ACamera>()) {
#ifndef NDEBUG
    RE_LOG(Log, "Selecting camera '%s' as sun camera / shadow projector.",
           name);
#endif
    if (pCamera->getProjectionType() != ECameraProjection::Orthogtaphic) {
      RE_LOG(Error,
             "Failed to set camera '%s' as sun camera. Camera must have an "
             "orthographic projection.",
             name);

      return;
    }

    view.pSunCamera = pCamera;
    view.pSunCamera->setLocation(core::actors.getLight(RLT_SUN)->getLocation());
    view.pSunCamera->setLookAtTarget(view.pActiveCamera, true, true);
    return;
  }

  RE_LOG(Error,
         "Failed to set sun / shadow projection camera '%s' - not found.",
         name);
}

void core::MRenderer::setSunCamera(ACamera* pCamera) {
  if (pCamera != nullptr) {
    if (pCamera->getProjectionType() != ECameraProjection::Orthogtaphic) {
      RE_LOG(Error,
             "Failed to set camera '%s' as sun camera. Camera must have an "
             "orthographic projection.",
             pCamera->getName());

      return;
    }

    view.pSunCamera = pCamera;
    view.pSunCamera->setLookAtTarget(view.pActiveCamera, true, true);
    return;
  }

  RE_LOG(Error,
         "Failed to set sun / shadow projection camera, received nullptr.");
}

ACamera* core::MRenderer::getCamera() { return view.pActiveCamera; }

void core::MRenderer::setIBLScale(float newScale) {
  lighting.data.scaleIBLAmbient = newScale;
  lighting.tracking.bufferUpdatesRemaining = MAX_FRAMES_IN_FLIGHT;
}
