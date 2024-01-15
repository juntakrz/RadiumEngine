#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/material/texture.h"
#include "core/managers/resources.h"

#include "stb_image.h"

TResult core::MResources::loadTexture(const std::string& filePath,
                                   RSamplerInfo* pSamplerInfo, const bool createDetailedViews) {
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
  RTexture* pNewTexture = &m_textures.at(filePath);
  pNewTexture->name = filePath;
  pNewTexture->isKTX = true;

  ktxResult = ktxTexture_VkUploadEx(
      pKTXTexture, deviceInfo, &pNewTexture->texture, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  if (ktxResult != KTX_SUCCESS) {
    RE_LOG(Error, "Failed uploading texture to GPU. KTX error %d.", ktxResult);
    revert(filePath.c_str());

    return RE_ERROR;
  }
  
  ktxTexture_Destroy(pKTXTexture);
  ktxVulkanDeviceInfo_Destruct(deviceInfo);

  if (pNewTexture->createImageViews(createDetailedViews) != RE_OK) {
    revert(filePath.c_str());
    return RE_ERROR;
  }

  if (pNewTexture->createSampler(pSamplerInfo) != RE_OK) {
    revert(filePath.c_str());
    return RE_ERROR;
  }

  if (pSamplerInfo) {
    if (pNewTexture->createDescriptor() != RE_OK) {
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

  RTextureInfo textureInfo{};
  textureInfo.name = fullPath;
  textureInfo.asCubemap = false;
  textureInfo.width = width;
  textureInfo.height = height;
  textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  textureInfo.memoryFlags = NULL;

  RTexture* pTexture = createTexture(&textureInfo);

  if (!pTexture) {
    RE_LOG(Error, "Failed to create texture \"%s\".", fullPath.c_str());
    return RE_ERROR;
  }

  TResult result = writeTexture(pTexture, pSTBI, imageSize);

  if (result != RE_OK) {
    RE_LOG(Error, "Failed to create texture \"%s\".", fullPath.c_str());
    return result;
  }

  RE_LOG(Log, "Successfully loaded PNG texture at \"%s\".", filePath.c_str());

  return result;
}

RTexture* core::MResources::createTexture(RTextureInfo* pInfo) {
  auto revert = [&](const char* name) { m_textures.erase(name); };

  if (pInfo->name == "") {
    // nothing to load
    return nullptr;
  }

  // create a texture record in the manager
  if (!m_textures.try_emplace(pInfo->name.c_str()).second) {
    // already loaded
#ifndef NDEBUG
    RE_LOG(Warning, "Texture \"%s\" already exists.", pInfo->name.c_str());
#endif
    return &m_textures.at(pInfo->name);
  }

  RTexture* newTexture = &m_textures.at(pInfo->name);
  VkResult result;
  VkDevice device = core::renderer.logicalDevice.device;

  VkImageCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  createInfo.imageType = VK_IMAGE_TYPE_2D;
  createInfo.format = pInfo->format;
  createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  createInfo.extent = {pInfo->width, pInfo->height, 1};
  createInfo.mipLevels = pInfo->mipLevels;
  createInfo.arrayLayers = pInfo->asCubemap ? 6u : pInfo->layerCount;
  createInfo.tiling = pInfo->tiling;
  createInfo.usage = pInfo->usageFlags;
  createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.flags = pInfo->asCubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : NULL;

  VmaAllocationCreateInfo allocCreateInfo{};
  allocCreateInfo.requiredFlags = pInfo->memoryFlags;
  allocCreateInfo.usage = pInfo->vmaMemoryUsage;

  result = vmaCreateImage(core::renderer.memAlloc, &createInfo,
                          &allocCreateInfo, &newTexture->texture.image,
                          &newTexture->allocation, &newTexture->allocationInfo);

  if (result != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create image for \"%s\"", pInfo->name.c_str());
    revert(pInfo->name.c_str());

    return nullptr;
  }

  newTexture->texture.imageFormat = pInfo->format;
  newTexture->texture.imageLayout = pInfo->targetLayout;

  VkImageSubresourceRange subRange;
  subRange.baseMipLevel = 0;
  subRange.levelCount = createInfo.mipLevels;
  subRange.baseArrayLayer = 0;
  subRange.layerCount = createInfo.arrayLayers;

  switch (pInfo->targetLayout) {
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
      subRange.aspectMask =
          VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      break;
    }
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL: {
      subRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      break;
    }
    default: {
      subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
  }

  VkCommandBuffer cmdBuffer = core::renderer.createCommandBuffer(
      ECmdType::Graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  core::renderer.setImageLayout(cmdBuffer, newTexture->texture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                pInfo->targetLayout, subRange);
  core::renderer.flushCommandBuffer(cmdBuffer, ECmdType::Graphics, true);

  newTexture->name = pInfo->name;
  newTexture->texture.imageFormat = createInfo.format;
  newTexture->texture.width = createInfo.extent.width;
  newTexture->texture.height = createInfo.extent.height;
  newTexture->texture.depth = 1;
  newTexture->texture.levelCount = createInfo.mipLevels;
  newTexture->texture.layerCount = createInfo.arrayLayers;

  if (newTexture->createImageViews(pInfo->detailedViews) != RE_OK) {
    revert(pInfo->name.c_str());
    return nullptr;
  }

  RSamplerInfo samplerInfo{};
  RSamplerInfo* pSamplerInfo = &samplerInfo;

  if (newTexture->createSampler(pSamplerInfo) != RE_OK) {
    revert(pInfo->name.c_str());
    return nullptr;
  }

  if (newTexture->createDescriptor() != RE_OK) {
    revert(pInfo->name.c_str());
    return nullptr;
  }

  std::string textureType = pInfo->asCubemap ? "cubemap" : "2D";

  RE_LOG(Log, "Successfully created %s texture \"%s\".", textureType.c_str(),
         pInfo->name.c_str());

  return newTexture;
}

TResult core::MResources::writeTexture(RTexture* pTexture, void* pData,
                                       VkDeviceSize dataSize) {
  if (!pData ||
      pTexture->texture.imageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    return RE_ERROR;
  }

  RBuffer staging;
  core::renderer.createBuffer(EBufferType::STAGING, dataSize, staging,
                              pData);
  core::renderer.copyBufferToImage(
      staging.buffer, pTexture->texture.image, pTexture->texture.width,
      pTexture->texture.height, pTexture->texture.layerCount);

  vmaDestroyBuffer(core::renderer.memAlloc, staging.buffer,
                   staging.allocation);

  VkImageSubresourceRange subRange{};
  subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subRange.baseMipLevel = 0;
  subRange.baseArrayLayer = 0;
  subRange.layerCount = pTexture->texture.layerCount;
  subRange.levelCount = pTexture->texture.levelCount;

  VkCommandBuffer cmdBuffer = core::renderer.createCommandBuffer(
      ECmdType::Graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  core::renderer.setImageLayout(cmdBuffer, pTexture->texture.image,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                subRange);

  pTexture->texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  core::renderer.flushCommandBuffer(cmdBuffer, ECmdType::Graphics, true);

  return RE_OK;
}

bool core::MResources::destroyTexture(const char* name, bool force) noexcept {
  // TODO: maybe should check if texture is used by a material here and avoid
  // destroying it unless explicitly forced

  // TODO: make sure it is actually deleted from memory
  if (m_textures.find(name) != m_textures.end()) {
    m_textures.erase(name);

#ifndef NDEBUG
    RE_LOG(Log, "Destroyed texture '%s'.", name);
#endif

    return true;
  }

  return false;
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
    updateMaterialDescriptorSet(pTexture, EResourceType::Sampler2D);
    pTexture->references++;

    return pTexture;
  }

  return nullptr;
}

void core::MResources::destroyAllTextures() { m_textures.clear(); }