#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/materials.h"

void core::MMaterials::transitionImageLayout(VkImage image, VkFormat format,
                                             VkImageLayout oldLayout,
                                             VkImageLayout newLayout) {

  VkCommandBuffer cmdBuffer = core::renderer.createCommandBuffer(
      ECmdType::Transfer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

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
  } else {
    RE_LOG(Error, "Unsupported layout transition.");
    return;
  }

  vkCmdPipelineBarrier(cmdBuffer, srcStageFlags, dstStageFlags, NULL, NULL,
                       nullptr, NULL, nullptr, 1, &imageBarrier);

  core::renderer.flushCommandBuffer(cmdBuffer, ECmdType::Transfer, true);
}

void core::MMaterials::loadTexture(const std::string& filePath,
                                   const RSamplerInfo* pSamplerInfo) {
  auto revert = [&](const char* name) { m_textures.erase(name); };

  if (filePath == "") {
    // nothing to load
    return;
  }

  // generate texture name from path - no texture file can be named the same
  // size_t with npos will overflow to 0 when 1 is added, which is good
  size_t folderPosition = filePath.find_last_of("/\\");
  size_t delimiterPosition = filePath.rfind('.');

  if (delimiterPosition == std::string::npos) {
    RE_LOG(Warning, "Delimiter is missing in file path \"%s\".",
           filePath.c_str());
    delimiterPosition = filePath.size() - 1;
  }

  std::string textureName(filePath, folderPosition + 1,
                          delimiterPosition - (folderPosition + 1));

  ktxTexture* pKTXTexture = nullptr;
  KTX_error_code ktxResult;
  RVkLogicalDevice* logicalDevice = &core::renderer.logicalDevice;

  ktxVulkanDeviceInfo* deviceInfo = ktxVulkanDeviceInfo_Create(
      core::renderer.physicalDevice.device, logicalDevice->device,
      logicalDevice->queues.transfer,
      core::renderer.getCommandPool(ECmdType::Transfer), nullptr);

  if (!deviceInfo) {
    RE_LOG(Error, "Failed to retrieve Vulkan device info.");
    return;
  }

  ktxResult = ktxTexture_CreateFromNamedFile(
      filePath.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &pKTXTexture);

  if (ktxResult != KTX_SUCCESS) {
    RE_LOG(Error, "Failed reading texture. KTX error %d.", ktxResult);
    return;
  }

  // create a texture record in the manager
  if (!m_textures.try_emplace(textureName).second) {
    // already loaded
#ifndef NDEBUG
    RE_LOG(Warning, "Texture \"%s\" already exists.", textureName.c_str());
#endif
    return;
  }

  // prepare freshly created texture structure
  RTexture* newTexture = &m_textures.at(textureName);
  newTexture->name = textureName;
  newTexture->filePath = filePath;
  newTexture->isKTX = true;

  ktxResult = ktxTexture_VkUploadEx(
      pKTXTexture, deviceInfo, &newTexture->texture, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  if (ktxResult != KTX_SUCCESS) {
    RE_LOG(Error, "Failed uploading texture to GPU. KTX error %d.", ktxResult);
    revert(textureName.c_str());

    return;
  }

  ktxTexture_Destroy(pKTXTexture);
  ktxVulkanDeviceInfo_Destruct(deviceInfo);

  if (newTexture->createImageView() != RE_OK) {
    return;
  }

  if (newTexture->createSampler(pSamplerInfo) != RE_OK) {
    return;
  }

  if (pSamplerInfo) {
    if (newTexture->createDescriptor() != RE_OK) {
      return;
    }
  }

  RE_LOG(Log, "Successfully loaded texture \"%s\".", textureName.c_str());
  /*
  // prepare data for loading texture into the graphics card memory
  // assume KTX1 uses sRGB format instead of linear (most tools use this)
  VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

  // using staging buffer (possibly test if it's not needed after all)
  bool useStagingBuffer = true;
  bool forceLinearTiling = false;

  // don't use format if linear shader sampling is not supported
  if (forceLinearTiling) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(core::renderer.physicalDevice.device,
                                        format, &formatProperties);
    useStagingBuffer = !(formatProperties.linearTilingFeatures &
                    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
  }

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

  VkMemoryRequirements memoryRequirements{};

  if (useStagingBuffer) {
    RBuffer stagingBuffer{};
    if (core::renderer.createBuffer(EBufferMode::STAGING,
                                    newTextureData.dataSize, stagingBuffer,
                                    nullptr) != RE_OK) {
      RE_LOG(Error, "Failed to create staging buffer for the new texture.");
      revert(textureName.c_str());
    }

    vkGetBufferMemoryRequirements(core::renderer.logicalDevice.device,
                                  stagingBuffer.buffer, &memoryRequirements);
  }*/
}

void core::MMaterials::createTexture(const char* name, uint32_t width, uint32_t height,
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

void core::MMaterials::destroyAllTextures() { m_textures.clear(); }

// RTEXTURE

TResult core::MMaterials::RTexture::createImageView() {
  texture.view =
      core::renderer.createImageView(texture.image, texture.imageFormat,
                                     texture.levelCount, texture.layerCount);

  if (!texture.view) {
    RE_LOG(Error, "Failed to create view for texture \"%s\"", name.c_str());
    return RE_ERROR;
  }

  return RE_OK;
}

TResult core::MMaterials::RTexture::createSampler(const RSamplerInfo* pSamplerInfo) {
  if (!pSamplerInfo) {
    RE_LOG(Warning, "Sampler info is missing, skipping sampler creation.");
    return RE_WARNING;
  }

  VkSamplerCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  createInfo.addressModeU = pSamplerInfo->addressModeU;
  createInfo.addressModeV = pSamplerInfo->addressModeV;
  createInfo.addressModeW = pSamplerInfo->addressModeW;
  createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  createInfo.minFilter = pSamplerInfo->minFilter;
  createInfo.magFilter = pSamplerInfo->magFilter;
  createInfo.anisotropyEnable = VK_TRUE;
  createInfo.maxAnisotropy = 16.0f;
  createInfo.compareOp = VK_COMPARE_OP_NEVER;
  createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  createInfo.maxLod = (float)texture.levelCount;

  if (vkCreateSampler(core::renderer.logicalDevice.device, &createInfo, nullptr,
                      &texture.sampler) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create sampler for texture \"%s\"", name.c_str());
    return RE_ERROR;
  };

  return RE_OK;
}

TResult core::MMaterials::RTexture::createDescriptor() {
  if (texture.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED || !texture.view ||
      !texture.sampler) {
    RE_LOG(Error,
           "Failed to create image descriptor. Make sure image layout is valid "
           "and both image view and sampler were created.");
    return RE_ERROR;
  }

  texture.descriptor.imageLayout = texture.imageLayout;
  texture.descriptor.imageView = texture.view;
  texture.descriptor.sampler = texture.sampler;

  return RE_OK;
}

core::MMaterials::RTexture::~RTexture() {
  VkDevice device = core::renderer.logicalDevice.device;
  vkDestroyImageView(device, texture.view, nullptr);
  vkDestroySampler(device, texture.sampler, nullptr);

  if (isKTX) {
    ktxVulkanTexture_Destruct(&texture, device, nullptr);
    return;
  }

  vmaDestroyImage(core::renderer.memAlloc, texture.image, allocation);
}