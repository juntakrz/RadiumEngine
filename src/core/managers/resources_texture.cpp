#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/material/texture.h"
#include "core/managers/resources.h"

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