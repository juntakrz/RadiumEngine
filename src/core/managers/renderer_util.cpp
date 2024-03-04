#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/animations.h"
#include "core/managers/ref.h"
#include "core/managers/renderer.h"
#include "core/managers/actors.h"
#include "core/managers/time.h"
#include "core/material/texture.h"
#include "core/model/model.h"
#include "core/world/actors/camera.h"

// PRIVATE

RTexture* core::MRenderer::createFragmentRenderTarget(const char* name, VkFormat format, uint32_t width, uint32_t height,
                                                      VkSamplerAddressMode addressMode) {
  if (width == 0 || height == 0) {
    width = swapchain.imageExtent.width;
    height = swapchain.imageExtent.height;
  }

  RTextureInfo textureInfo{};
  textureInfo.name = name;
  textureInfo.layerCount = 1u;
  textureInfo.format = format;
  textureInfo.width = width;
  textureInfo.height = height;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  textureInfo.samplerInfo.addressMode = addressMode;

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

TResult core::MRenderer::createViewports() {
  system.viewports.resize(EViewport::vpCount);

  VkViewport viewport{};
  viewport.x = 0;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = { 0, 0 };

  // EnvSkybox
  viewport.y = static_cast<float>(core::vulkan::envFilterExtent);
  viewport.width = static_cast<float>(core::vulkan::envFilterExtent);
  viewport.height = -static_cast<float>(core::vulkan::envFilterExtent);
  scissor.extent = { core::vulkan::envFilterExtent, core::vulkan::envFilterExtent };
  system.viewports.at(EViewport::vpEnvironment).viewport = viewport;
  system.viewports.at(EViewport::vpEnvironment).scissor = scissor;

  // EnvIrrad
  viewport.y = static_cast<float>(core::vulkan::envIrradianceExtent);
  viewport.width = static_cast<float>(core::vulkan::envIrradianceExtent);
  viewport.height = -static_cast<float>(core::vulkan::envIrradianceExtent);
  scissor.extent = { core::vulkan::envIrradianceExtent, core::vulkan::envIrradianceExtent };
  system.viewports.at(EViewport::vpEnvIrrad).viewport = viewport;
  system.viewports.at(EViewport::vpEnvIrrad).scissor = scissor;

  // Shadow
  viewport.y = static_cast<float>(config::shadowResolution);
  viewport.width = static_cast<float>(config::shadowResolution);
  viewport.height = -static_cast<float>(config::shadowResolution);
  scissor.extent = { config::shadowResolution, config::shadowResolution };
  system.viewports.at(EViewport::vpShadow).viewport = viewport;
  system.viewports.at(EViewport::vpShadow).scissor = scissor;

  // Main
  viewport.y = static_cast<float>(config::renderHeight);
  viewport.width = static_cast<float>(config::renderWidth);
  viewport.height = -static_cast<float>(config::renderHeight);
  scissor.extent = { config::renderWidth, config::renderHeight };
  system.viewports.at(EViewport::vpMain).viewport = viewport;
  system.viewports.at(EViewport::vpMain).scissor = scissor;

  return RE_OK;
}

void core::MRenderer::setViewport(VkCommandBuffer commandBuffer, EViewport index) {
  vkCmdSetViewport(commandBuffer, 0, 1, &system.viewports[index].viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &system.viewports[index].scissor);

  renderView.currentViewportId = index;
}

RViewport* core::MRenderer::getViewportData(EViewport viewportId) {
  return &system.viewports.at(viewportId);
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

TResult core::MRenderer::getDepthStencilFormat(VkFormat desiredFormat, VkFormat& outFormat) {
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT
  };

  VkFormatProperties formatProps;
  vkGetPhysicalDeviceFormatProperties(physicalDevice.device, desiredFormat, &formatProps);
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
    vkGetPhysicalDeviceFormatProperties(physicalDevice.device, format, &formatProps);
    if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      outFormat = format;
      return RE_WARNING;
    }
  }

  RE_LOG(Critical, "Failed to find an appropriate depth/stencil format.");
  return RE_CRITICAL;
}


VkPipelineShaderStageCreateInfo core::MRenderer::loadShader(const char* path, VkShaderStageFlagBits stage) {
  std::string fullPath = RE_PATH_SHADERS + std::string(path);
  std::vector<char> shaderCode = util::readFile(fullPath.c_str());

  VkPipelineShaderStageCreateInfo stageCreateInfo{};
  stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageCreateInfo.stage = stage;
  stageCreateInfo.module = createShaderModule(shaderCode);
  stageCreateInfo.pName = "main";

  return stageCreateInfo;
}

VkShaderModule core::MRenderer::createShaderModule(std::vector<char>& shaderCode) {
  VkShaderModuleCreateInfo smInfo{};
  smInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  smInfo.codeSize = shaderCode.size();
  smInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());

  VkShaderModule shaderModule;
  if ((vkCreateShaderModule(logicalDevice.device, &smInfo, nullptr, &shaderModule) != VK_SUCCESS)) {
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
  checkResult = vkEnumerateInstanceLayerProperties(&layerCount, availableValidationLayers.data());
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

  if (requireValidationLayers) {
    requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return requiredExtensions;
}

std::vector<VkExtensionProperties> core::MRenderer::getInstanceExtensions(const char* layerName,
                                                                          const char* extensionToCheck,
                                                                          bool* pCheckResult) {
  uint32_t extensionCount = 0;
  std::vector<VkExtensionProperties> extensionProperties;

  vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr);
  extensionProperties.resize(extensionCount);
  vkEnumerateInstanceExtensionProperties(layerName, &extensionCount,
                                         extensionProperties.data());

  // check if extension is available
  if (extensionToCheck != nullptr && pCheckResult != nullptr) {
    *pCheckResult = false;

    for (auto& it : extensionProperties) {
      if (strcmp(it.extensionName, extensionToCheck) == 0) {
        *pCheckResult = true;
        break;
      }
    }
  }

  return extensionProperties;
}

TResult core::MRenderer::createQueryPool() {
  VkQueryPoolCreateInfo poolCreate{};
  poolCreate.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  poolCreate.queryType = VK_QUERY_TYPE_OCCLUSION;
  poolCreate.queryCount = config::scene::nodeBudget;    // TODO: Node budget is quite large, but find a more valid number
  
  if (vkCreateQueryPool(logicalDevice.device, &poolCreate, nullptr, &system.queryPool) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create query pool.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroyQueryPool() {
  vkDestroyQueryPool(logicalDevice.device, system.queryPool, nullptr);
}

// Runs in a dedicated thread
void core::MRenderer::updateBoundEntities() {
  AEntity* pEntity = nullptr;

  // Update animation matrices
  core::animations.runAnimationQueue();

  for (auto& bindInfo : system.bindings) {
    if ((pEntity = bindInfo.pEntity) == nullptr) {
      continue;
    }

    // Update model matrices
    pEntity->updateModel();
  }

  // Use this thread to also quickly process camera exposure level
  updateExposureLevel();
}

void core::MRenderer::updateExposureLevel() {
  const float deltaTime = core::time.getDeltaTime();
  float brightnessData[256];
  memcpy(brightnessData, postprocess.exposureStorageBuffer.allocInfo.pMappedData, 1024);

  float averageLuminance = 0.0f;
  for (int i = 0; i < 256; ++i) {
    brightnessData[i] = (brightnessData[i] - 0.25f) * 2.0f + 0.25f;
    brightnessData[i] = std::min(1.5f, std::max(brightnessData[i], 0.0f));
    averageLuminance += brightnessData[i];
  }

  averageLuminance /= 256.0f;

  if (lighting.data.averageLuminance < averageLuminance - 0.01f ||
    lighting.data.averageLuminance > averageLuminance + 0.01f) {
    lighting.data.averageLuminance +=
      (lighting.data.averageLuminance < averageLuminance)
      ? deltaTime * 0.25f
      : -deltaTime * 0.25f;
  }
}

// PUBLIC

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
                 &copyRegion);

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
  if (pTexture->texture.imageLayout == newLayout) return;

  setImageLayout(cmdBuffer, pTexture->texture.image,
                 pTexture->texture.imageLayout, newLayout, subresourceRange);
  
  pTexture->texture.imageLayout = newLayout;
  pTexture->texture.imageInfo.imageLayout = newLayout;
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
    case VK_IMAGE_LAYOUT_GENERAL:
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

    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
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

    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
    imageMemoryBarrier.dstAccessMask = 0;
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

    case VK_IMAGE_LAYOUT_GENERAL:
    imageMemoryBarrier.dstAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    break;

    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
    imageMemoryBarrier.dstAccessMask = 0;
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

TResult core::MRenderer::generateMipMaps(VkCommandBuffer cmdBuffer, RTexture* pTexture, int32_t mipLevels, VkFilter filter) {
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

  pTexture->texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  pTexture->texture.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

  switch (pTexture->texture.imageInfo.imageLayout) {
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
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
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

    vkCmdPipelineBarrier(cmdBuffer, srcStageMask, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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

  vkCmdBlitImage(cmdBuffer, pTexture->texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pTexture->texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);

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

      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.subresourceRange.baseMipLevel = mipLevel;

      break;
    }
  }

  vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  return RE_OK;
}

TResult core::MRenderer::createDefaultSamplers() {
  // 0. Linear / Repeat
  material.samplers.emplace_back();

  VkSamplerCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  createInfo.minFilter = VK_FILTER_LINEAR;
  createInfo.magFilter = VK_FILTER_LINEAR;
  createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.anisotropyEnable = VK_TRUE;
  createInfo.maxAnisotropy = config::maxAnisotropy;
  createInfo.compareOp = VK_COMPARE_OP_NEVER;
  createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  createInfo.maxLod = 16.0f;

  if (vkCreateSampler(core::renderer.logicalDevice.device, &createInfo, nullptr,
    &material.samplers.back()) != VK_SUCCESS) {
    return RE_ERROR;
  };

  // 1. Linear / Clamp to edge
  material.samplers.emplace_back();

  createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  if (vkCreateSampler(core::renderer.logicalDevice.device, &createInfo, nullptr,
    &material.samplers.back()) != VK_SUCCESS) {
    return RE_ERROR;
  };

  // 2. Nearest / Repeat
  material.samplers.emplace_back();

  createInfo.minFilter = VK_FILTER_NEAREST;
  createInfo.magFilter = VK_FILTER_NEAREST;
  createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  if (vkCreateSampler(core::renderer.logicalDevice.device, &createInfo, nullptr,
    &material.samplers.back()) != VK_SUCCESS) {
    return RE_ERROR;
  };

  // 3. Nearest / Clamp to edge
  material.samplers.emplace_back();

  createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  if (vkCreateSampler(core::renderer.logicalDevice.device, &createInfo, nullptr,
    &material.samplers.back()) != VK_SUCCESS) {
    return RE_ERROR;
  };

  return RE_OK;
}

void core::MRenderer::destroySamplers() {
  for (auto& it : material.samplers) {
    vkDestroySampler(logicalDevice.device, it, nullptr);
  }
}

VkSampler core::MRenderer::getSampler(RSamplerInfo* pInfo) {
  if (!pInfo) {
    return material.samplers[0];
  }

  if ((pInfo->addressMode == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) && (pInfo->filter = VK_FILTER_LINEAR)) {
    return material.samplers[1];
  }
  else if ((pInfo->addressMode == VK_SAMPLER_ADDRESS_MODE_REPEAT) && (pInfo->filter = VK_FILTER_NEAREST)) {
    return material.samplers[2];
  }
  else if ((pInfo->addressMode == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) && (pInfo->filter = VK_FILTER_NEAREST)) {
    return material.samplers[3];
  }

  // Return default repeat / linear sampler
  return material.samplers[0];
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

VkCommandBuffer core::MRenderer::createCommandBuffer(ECmdType type, VkCommandBufferLevel level, bool begin) {
  VkCommandBuffer newCommandBuffer;
  VkCommandBufferAllocateInfo allocateInfo{};

  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = getCommandPool(type);
  allocateInfo.commandBufferCount = 1;
  allocateInfo.level = level;

  vkAllocateCommandBuffers(logicalDevice.device, &allocateInfo, &newCommandBuffer);

  if (begin) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(newCommandBuffer, &beginInfo);
  }

  return newCommandBuffer;
}

void core::MRenderer::beginCommandBuffer(VkCommandBuffer cmdBuffer, bool oneTimeSubmit) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (oneTimeSubmit) {
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  }

  vkBeginCommandBuffer(cmdBuffer, &beginInfo);
}

void core::MRenderer::flushCommandBuffer(VkCommandBuffer cmdBuffer, ECmdType type, bool free, bool useFence) {
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
    vkFreeCommandBuffers(logicalDevice.device, getCommandPool(type), 1, &cmdBuffer);
  }
}

VkImageView core::MRenderer::createImageView(VkImage image, VkFormat format, uint32_t baseLayer, uint32_t layerCount,
                                             uint32_t baseLevel, uint32_t levelCount, const bool isCubemap, VkImageAspectFlags aspectMask) {
  VkImageView imageView = nullptr;

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.format = format;

  if (isCubemap) {
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

  viewInfo.subresourceRange.aspectMask = aspectMask;
  viewInfo.subresourceRange.baseMipLevel = baseLevel;
  viewInfo.subresourceRange.levelCount = levelCount;
  viewInfo.subresourceRange.baseArrayLayer = (isCubemap) ? 0u : baseLayer;
  viewInfo.subresourceRange.layerCount = (isCubemap) ? 6u : layerCount;

  if (vkCreateImageView(logicalDevice.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
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

  // add model to rendering queue, store its offsets
  REntityBindInfo bindInfo{};
  bindInfo.pEntity = pEntity;

  system.bindings.emplace_back(bindInfo);

  WModel* pModel = pEntity->getModel();

  // Add a reference to all WModel entries of AEntity if they don't already exist
  if (scene.pModelReferences.find(pModel) == scene.pModelReferences.end()) {
    scene.pModelReferences.emplace(pModel);
  }

  // Add a number of model primitives to total primitive instances rendered
  scene.totalInstances += pModel->m_pLinearPrimitives.size();

  for (auto& primitive : pModel->m_pLinearPrimitives) {
    WModel::Node* pNode = reinterpret_cast<WModel::Node*>(primitive->pOwnerNode);

    auto& instanceData = primitive->instanceData.emplace_back();
    instanceData.instanceIndex.resize(MAX_FRAMES_IN_FLIGHT);
    instanceData.isVisible = true;
    instanceData.pParentEntity = pEntity;
    instanceData.instanceBufferBlock.modelMatrixId = pEntity->getRootTransformBufferIndex();
    instanceData.instanceBufferBlock.nodeMatrixId = pEntity->getNodeTransformBufferIndex(pNode->index);
    instanceData.instanceBufferBlock.skinMatrixId = pEntity->getSkinTransformBufferIndex(pNode->skinIndex);
    instanceData.instanceBufferBlock.materialId = primitive->pInitialMaterial->bufferIndex;
  }

  pEntity->setRendererBindingIndex(
      static_cast<int32_t>(system.bindings.size() - 1));

#ifndef NDEBUG
  RE_LOG(Log, "Bound model \"%s\" to graphics pipeline.", pModel->getName());
#endif

  return (uint32_t)system.bindings.size() - 1;
}

void core::MRenderer::unbindEntity(uint32_t index) {
  if (index > system.bindings.size() - 1) {
    RE_LOG(Error, "Failed to unbind entity at %d. Index is out of bounds.", index);
    return;
  }

#ifndef NDEBUG
  if (system.bindings[index].pEntity == nullptr) {
    RE_LOG(Warning, "Failed to unbind entity at %d. It's already unbound.", index);
    return;
  }
#endif

  system.bindings[index].pEntity->setRendererBindingIndex(-1);
  system.bindings[index].pEntity = nullptr;
}

void core::MRenderer::clearBoundEntities() { system.bindings.clear(); }

void core::MRenderer::uploadModelToSceneBuffer(WModel* pModel) {
  // Copy vertex buffer
  VkBufferCopy copyInfo{};
  copyInfo.srcOffset = 0u;
  copyInfo.dstOffset = scene.currentVertexOffset * sizeof(RVertex);
  copyInfo.size = sizeof(RVertex) * pModel->m_vertexCount;

  copyBuffer(&pModel->staging.vertexBuffer, &scene.vertexBuffer, &copyInfo);

  // Copy index buffer
  copyInfo.dstOffset = scene.currentIndexOffset * sizeof(uint32_t);
  copyInfo.size = sizeof(uint32_t) * pModel->m_indexCount;

  copyBuffer(&pModel->staging.indexBuffer, &scene.indexBuffer, &copyInfo);

  // Set offsets in the model for future reference
  pModel->m_sceneVertexOffset = scene.currentVertexOffset;
  pModel->m_sceneIndexOffset = scene.currentIndexOffset;

  // Remove model staging data from memory
  pModel->clearStagingData();

  // Move scene offsets
  scene.currentVertexOffset += pModel->m_vertexCount;
  scene.currentIndexOffset += pModel->m_indexCount;
}

void core::MRenderer::setCamera(const char* name, const bool setAsPrimary) {
  ACamera* pCamera = core::actors.getCamera(name);

  if (!pCamera) {
    RE_LOG(Error, "Failed to set camera '%s' - not found.", name);
    return;
  }

  view.pActiveCamera = pCamera;

  if (setAsPrimary) {
    view.pPrimaryCamera = pCamera;
  }
}

void core::MRenderer::setCamera(ACamera* pCamera, const bool setAsPrimary) {
  if (!pCamera) {
    RE_LOG(Error, "Failed to set camera, received nullptr.");
    return;
  }

  view.pActiveCamera = pCamera;

  if (setAsPrimary) {
    view.pPrimaryCamera = pCamera;
  }
}

void core::MRenderer::setSunCamera(const char* name) {
  if (ACamera* pCamera = core::ref.getActor(name)->getAs<ACamera>()) {
#ifndef NDEBUG
    RE_LOG(Log, "Selecting camera '%s' as sun camera / shadow projector.", name);
#endif
    if (pCamera->getProjectionType() != ECameraProjection::Orthogtaphic) {
      RE_LOG(Error, "Failed to set camera '%s' as sun camera. Camera must have an orthographic projection.", name);

      return;
    }

    view.pSunCamera = pCamera;
    view.pSunCamera->setLocation(core::actors.getLight(RLT_SUN)->getLocation());
    view.pSunCamera->setLookAtTarget(view.pActiveCamera, true, true);
    return;
  }

  RE_LOG(Error, "Failed to set sun / shadow projection camera '%s' - not found.", name);
}

void core::MRenderer::setSunCamera(ACamera* pCamera) {
  if (pCamera != nullptr) {
    if (pCamera->getProjectionType() != ECameraProjection::Orthogtaphic) {
      RE_LOG(Error, "Failed to set camera '%s' as sun camera. Camera must have an orthographic projection.", pCamera->getName());
      return;
    }

    view.pSunCamera = pCamera;
    view.pSunCamera->setLookAtTarget(view.pActiveCamera, true, true);
    return;
  }

  RE_LOG(Error, "Failed to set sun / shadow projection camera, received nullptr.");
}

ACamera* core::MRenderer::getCamera() { return view.pActiveCamera; }

void core::MRenderer::setIBLScale(float newScale) {
  lighting.data.scaleIBLAmbient = newScale;
}

void core::MRenderer::setShadowColor(const glm::vec3& color) {
  lighting.data.shadowColor.x = color.x;
  lighting.data.shadowColor.y = color.y;
  lighting.data.shadowColor.z = color.z;
}

void core::MRenderer::setBloomIntensity(const float intensity) {
  lighting.data.bloomIntensity = intensity;
}