#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/material/texture.h"
#include "core/managers/resources.h"

#include "stb_image.h"

TResult core::MResources::loadTexture(const std::string& filePath,
                                   RSamplerInfo* pSamplerInfo) {
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

TResult core::MResources::loadTextureToBuffer(const std::string& filePath,
                                              RBuffer& outBuffer) {
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

  return RE_OK;
}

TResult core::MResources::loadTexturePNG(const std::string& filePath,
                                         RSamplerInfo* pSamplerInfo) {
  auto revert = [&](const char* name) { m_textures.erase(name); };

  if (filePath == "") {
    // nothing to load
    return RE_WARNING;
  }

  std::string fullPath = RE_PATH_TEXTURES + filePath;
  int width, height, channels = 0;
  stbi_uc* pSTBI =
      stbi_load(fullPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  VkDeviceSize imageSize = width * height * 4;

  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

  TResult result = createTexture(fullPath.c_str(), width, height, format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                false, pSTBI, imageSize, nullptr);

  RE_LOG(Log, "Successfully loaded PNG texture at \"%s\".", filePath.c_str());

  return result;
}

TResult core::MResources::createTexture(const char* name, uint32_t width,
                                        uint32_t height, VkFormat format,
                                        VkImageTiling tiling,
                                        VkImageUsageFlags usage, bool asCubemap,
                                        void* pData, VkDeviceSize inDataSize,
                                        VkMemoryPropertyFlags* properties) {
  auto revert = [&](const char* name) { m_textures.erase(name); };

  if (name == "") {
    // nothing to load
    return RE_ERROR;
  }

  // create a texture record in the manager
  if (!m_textures.try_emplace(name).second) {
    // already loaded
#ifndef NDEBUG
    RE_LOG(Warning, "Texture \"%s\" already exists.", name);
#endif
    return RE_WARNING;
  }

  RTexture* newTexture = &m_textures.at(name);
  VkResult result;
  VkDevice device = core::renderer.logicalDevice.device;

  VkImageCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  createInfo.imageType = VK_IMAGE_TYPE_2D;
  createInfo.format = format;
  createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  createInfo.extent = {width, height, 1};
  createInfo.mipLevels = 1;
  createInfo.arrayLayers = asCubemap ? 6u : 1u;
  createInfo.tiling = tiling;
  createInfo.usage = usage;
  createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocCreateInfo{};
  if (properties) {
    allocCreateInfo.requiredFlags = *properties;
  }

  result = vmaCreateImage(core::renderer.memAlloc, &createInfo,
                          &allocCreateInfo, &newTexture->texture.image,
                          &newTexture->allocation, &newTexture->allocationInfo);

  if (result != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create image for \"%s\"", name);
    revert(name);

    return RE_ERROR;
  }

  newTexture->texture.imageFormat = format;

  VkImageSubresourceRange subRange;
  subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subRange.baseMipLevel = 0;
  subRange.levelCount = createInfo.mipLevels;
  subRange.baseArrayLayer = 0;
  subRange.layerCount = createInfo.arrayLayers;

  VkCommandBuffer cmdBuffer = core::renderer.createCommandBuffer(
      ECmdType::Graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  core::renderer.setImageLayout(cmdBuffer, newTexture->texture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subRange);
  newTexture->texture.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

  if (pData) {
    core::renderer.flushCommandBuffer(cmdBuffer, ECmdType::Graphics);

    RBuffer staging;
    core::renderer.createBuffer(EBufferMode::STAGING, inDataSize, staging,
                                pData);
    core::renderer.copyBufferToImage(staging.buffer, newTexture->texture.image,
                                     width, height, createInfo.arrayLayers);

    vmaDestroyBuffer(core::renderer.memAlloc, staging.buffer,
                     staging.allocation);

    core::renderer.beginCommandBuffer(cmdBuffer);
    core::renderer.setImageLayout(cmdBuffer, newTexture->texture.image,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  subRange);
    newTexture->texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  core::renderer.flushCommandBuffer(cmdBuffer, ECmdType::Graphics, true);

  newTexture->name = name;
  newTexture->texture.imageFormat = createInfo.format;
  newTexture->texture.width = createInfo.extent.width;
  newTexture->texture.height = createInfo.extent.height;
  newTexture->texture.depth = 1;
  newTexture->texture.levelCount = createInfo.mipLevels;
  newTexture->texture.layerCount = createInfo.arrayLayers;

  if (newTexture->createImageView() != RE_OK) {
    revert(name);
    return RE_ERROR;
  }

  RSamplerInfo samplerInfo{};
  RSamplerInfo* pSamplerInfo = &samplerInfo;

  if (newTexture->createSampler(pSamplerInfo) != RE_OK) {
    revert(name);
    return RE_ERROR;
  }

  if (&samplerInfo) {
    if (newTexture->createDescriptor() != RE_OK) {
      revert(name);
      return RE_ERROR;
    }
  }

  std::string textureType = asCubemap ? "cubemap" : "2D";

  RE_LOG(Log, "Successfully created %s texture \"%s\".", textureType.c_str(),
         name);

  return RE_OK;
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