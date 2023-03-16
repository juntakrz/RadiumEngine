#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/material/texture.h"
#include "core/managers/resources.h"

#include "stb_image.h"

TResult core::MResources::loadTexture(const std::string& filePath,
                                   const RSamplerInfo* pSamplerInfo) {
  auto revert = [&](const char* name) { m_textures.erase(name); };

  if (filePath == "") {
    // nothing to load
    return RE_WARNING;
  }

  std::string fullPath = RE_PATH_TEXTURES + filePath;

  ktxTexture* pKTXTexture = nullptr;
  KTX_error_code ktxResult;
  RVkLogicalDevice* logicalDevice = &core::renderer.logicalDevice;

  ktxVulkanDeviceInfo* deviceInfo = ktxVulkanDeviceInfo_Create(
      core::renderer.physicalDevice.device, logicalDevice->device,
      logicalDevice->queues.transfer,
      core::renderer.getCommandPool(ECmdType::Transfer), nullptr);

  if (!deviceInfo) {
    RE_LOG(Error, "Failed to retrieve Vulkan device info.");
    return RE_ERROR;
  }

  ktxResult = ktxTexture_CreateFromNamedFile(
      fullPath.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &pKTXTexture);

  if (ktxResult != KTX_SUCCESS) {
    RE_LOG(Error, "Failed reading texture. KTX error %d.", ktxResult);
    return RE_ERROR;
  }

  // create a texture record in the manager
  if (!m_textures.try_emplace(filePath).second) {
    // already loaded
#ifndef NDEBUG
    RE_LOG(Warning, "Texture \"%s\" already exists.", filePath.c_str());
#endif
    return RE_WARNING;
  }

  // prepare freshly created texture structure
  RTexture* newTexture = &m_textures.at(filePath);
  newTexture->name = filePath;
  newTexture->isKTX = true;

  ktxResult = ktxTexture_VkUploadEx(
      pKTXTexture, deviceInfo, &newTexture->texture, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  if (ktxResult != KTX_SUCCESS) {
    RE_LOG(Error, "Failed uploading texture to GPU. KTX error %d.", ktxResult);
    revert(filePath.c_str());

    return RE_ERROR;
  }
  
  ktxTexture_Destroy(pKTXTexture);
  ktxVulkanDeviceInfo_Destruct(deviceInfo);

  if (newTexture->createImageView() != RE_OK) {
    revert(filePath.c_str());
    return RE_ERROR;
  }

  if (newTexture->createSampler(pSamplerInfo) != RE_OK) {
    revert(filePath.c_str());
    return RE_ERROR;
  }

  if (pSamplerInfo) {
    if (newTexture->createDescriptor() != RE_OK) {
      revert(filePath.c_str());
      return RE_ERROR;
    }
  }

  RE_LOG(Log, "Successfully loaded texture \"%s\".", filePath.c_str());

  return RE_OK;
}

TResult core::MResources::loadTexturePNG(const std::string& filePath,
                                         const RSamplerInfo* pSamplerInfo) {
  auto revert = [&](const char* name) { m_textures.erase(name); };

  if (filePath == "") {
    // nothing to load
    return RE_WARNING;
  }

  std::string fullPath = RE_PATH_TEXTURES + filePath;

  // create a texture record in the manager
  if (!m_textures.try_emplace(filePath).second) {
  // already loaded
#ifndef NDEBUG
    RE_LOG(Warning, "Texture \"%s\" already exists.", filePath.c_str());
#endif
    return RE_WARNING;
  }

  RTexture* newTexture = &m_textures.at(filePath);
  newTexture->name = filePath;
  newTexture->isKTX = false;

  int width, height, channels = 0;
  stbi_uc* pSTBI =
      stbi_load(fullPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  VkDeviceSize imageSize = width * height * 4;

  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

  RBuffer staging;
  core::renderer.createBuffer(EBufferMode::STAGING, imageSize, staging, pSTBI);

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32_t>(width);
  imageInfo.extent.height = static_cast<uint32_t>(height);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  VmaAllocationCreateInfo allocInfo{};
  //allocInfo.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VkResult result = vmaCreateImage(core::renderer.memAlloc, &imageInfo, &allocInfo,
                 &newTexture->texture.image, &newTexture->allocation,
                 &newTexture->allocationInfo);

  VkImageSubresourceRange subRange;
  subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subRange.baseMipLevel = 0;
  subRange.levelCount = 1;
  subRange.baseArrayLayer = 0;
  subRange.layerCount = 1;

  VkCommandBuffer cmdBuffer = core::renderer.createCommandBuffer(
      ECmdType::Graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  core::renderer.setImageLayout(cmdBuffer, newTexture->texture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subRange);
  core::renderer.flushCommandBuffer(cmdBuffer, ECmdType::Graphics);

  core::renderer.beginCommandBuffer(cmdBuffer);
  core::renderer.copyBufferToImage(staging.buffer, newTexture->texture.image,
                                   width, height);

  core::renderer.setImageLayout(cmdBuffer, newTexture->texture.image,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                subRange);
  core::renderer.flushCommandBuffer(cmdBuffer, ECmdType::Graphics, true);

  newTexture->texture.imageFormat = imageInfo.format;
  newTexture->texture.width = width;
  newTexture->texture.height = height;
  newTexture->texture.layerCount = 1;
  newTexture->texture.levelCount = 1;
  newTexture->texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  if (newTexture->createImageView() != RE_OK) {
    revert(filePath.c_str());
    return RE_ERROR;
  }

  RSamplerInfo samplerInfo{};
  pSamplerInfo = &samplerInfo;

  if (newTexture->createSampler(pSamplerInfo) != RE_OK) {
    revert(filePath.c_str());
    return RE_ERROR;
  }

  if (pSamplerInfo) {
    if (newTexture->createDescriptor() != RE_OK) {
      revert(filePath.c_str());
      return RE_ERROR;
    }
  }

  RE_LOG(Log, "Successfully loaded texture \"%s\".", filePath.c_str());

  return RE_OK;
}

void core::MResources::createTexture(const char* name, uint32_t width, uint32_t height,
  VkFormat format, VkImageTiling tiling,
  VkImageUsageFlags usage,
  RTexture* pTexture, VkMemoryPropertyFlags* properties) {

  auto revert = [&](const char* name) { m_textures.erase(name); };

  if (name == "") {
    // nothing to load
    return;
  }

    // create a texture record in the manager
  if (!m_textures.try_emplace(name).second) {
    // already loaded
#ifndef NDEBUG
    RE_LOG(Warning, "Texture \"%s\" already exists.", name);
#endif
    return;
  }

  VkResult result;
  VkDevice device = core::renderer.logicalDevice.device;

  VkImageCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  createInfo.imageType = VK_IMAGE_TYPE_2D;
  createInfo.format = format;
  createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  createInfo.extent = {width, height, 1};
  createInfo.mipLevels = 1;
  createInfo.arrayLayers = 1;
  createInfo.tiling = tiling;
  createInfo.usage = usage;
  createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (properties) {
    RE_LOG(Warning, "Image creation property flags are currently unused.");
  }

  result = vmaCreateImage(core::renderer.memAlloc, &createInfo, nullptr,
                          &pTexture->texture.image, &pTexture->allocation,
                          &pTexture->allocationInfo);

  if (result != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create image for \"%s\"", name);
    revert(name);

    return;
  }
  
  pTexture->texture.imageFormat = format;
  /*pTexture->texture.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  
  // transition image to layout optimal
  transitionImageLayout(pTexture->texture.image, pTexture->texture.imageFormat,
                        pTexture->texture.imageLayout,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                        */ 
}

RTexture* core::MResources::getTexture(
    const char* name) noexcept {
  if (m_textures.contains(name)) {
    return &m_textures.at(name);
  }

  return nullptr;
}

RTexture* const core::MResources::assignTexture(
    const char* name) noexcept {
  if (m_textures.contains(name)) {
    RTexture* pTexture = &m_textures.at(name);
    ++pTexture->references;
    return pTexture;
  }

  return nullptr;
}

void core::MResources::destroyAllTextures() { m_textures.clear(); }