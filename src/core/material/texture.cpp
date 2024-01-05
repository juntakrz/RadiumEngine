#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/material/texture.h"

TResult RTexture::createImageViews(const bool createDetailedViews) {
  // create complete image view
  texture.view =
      core::renderer.createImageView(texture.image, texture.imageFormat,
                                     texture.levelCount, texture.layerCount);

  // create separate views into every layer and mipmap
  if (createDetailedViews) {
    texture.layerAndMipViews.resize(texture.layerCount);

    for (auto& it : texture.layerAndMipViews) {
      it.resize(texture.levelCount);
    }

    for (uint8_t layerIndex = 0; layerIndex < texture.layerCount;
         ++layerIndex) {
      for (uint8_t mipIndex = 0; mipIndex < texture.levelCount; ++mipIndex) {
        texture.layerAndMipViews[layerIndex][mipIndex] =
            core::renderer.createImageView(texture.image, texture.imageFormat,
                                           1u, 1u, mipIndex, layerIndex);
      }
    }
  }
  
  texture.viewType = VK_IMAGE_VIEW_TYPE_2D;

  if (texture.layerCount == 6) {
    texture.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    isCubemap = true;
  }

  if (!texture.view) {
    RE_LOG(Error, "Failed to create view for texture \"%s\"", name.c_str());
    return RE_ERROR;
  }

  return RE_OK;
}

TResult RTexture::createSampler(
    RSamplerInfo* pSamplerInfo) {
  if (!pSamplerInfo) {
    RE_LOG(Warning, "Sampler info is missing, skipping sampler creation.");
    return RE_WARNING;
  }

  // adapt to cubemap
  if (texture.layerCount == 6) {
    pSamplerInfo->addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pSamplerInfo->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pSamplerInfo->addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  }

  VkSamplerCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  createInfo.minFilter = pSamplerInfo->minFilter;
  createInfo.magFilter = pSamplerInfo->magFilter;
  createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  createInfo.addressModeU = pSamplerInfo->addressModeU;
  createInfo.addressModeV = pSamplerInfo->addressModeV;
  createInfo.addressModeW = pSamplerInfo->addressModeW;
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

TResult RTexture::createDescriptor() {
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

RTexture::~RTexture() {
  VkDevice device = core::renderer.logicalDevice.device;
  vkDestroyImageView(device, texture.view, nullptr);
  vkDestroySampler(device, texture.sampler, nullptr);

  if (isKTX) {
    ktxVulkanTexture_Destruct(&texture, device, nullptr);
    return;
  }

  vmaDestroyImage(core::renderer.memAlloc, texture.image, allocation);
}