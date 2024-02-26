#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/material/texture.h"

TResult RTexture::createImageViews(const bool createExtraViews, const bool createCubemapFaceViews) {
  // Create complete image view
  texture.view =
      core::renderer.createImageView(texture.image, texture.imageFormat, 0u, texture.layerCount,
                                      0u, texture.levelCount, isCubemap, texture.aspectMask);

  texture.viewType = VK_IMAGE_VIEW_TYPE_2D;

  if (isCubemap && texture.layerCount == 6) {
    texture.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

    if (createCubemapFaceViews) {
      texture.cubemapFaceViews.resize(6);

      for (uint8_t i = 0; i < 6; ++i) {
        texture.cubemapFaceViews[i] =
          core::renderer.createImageView(texture.image, texture.imageFormat, i, 1u, 0u, 1u, false, texture.aspectMask);
      }
    }
  }

  // Create separate views into every layer and mipmap
  if (createExtraViews) {
    switch (isCubemap) {
      case true: {
        texture.extraViews.resize(texture.levelCount - 1);

        for (uint8_t mipIndex = 1; mipIndex < texture.levelCount; ++mipIndex) {
          VkDescriptorImageInfo& info = texture.extraViews[mipIndex - 1];

          info.imageView = core::renderer.createImageView(texture.image, texture.imageFormat, 0u, 6u, mipIndex, 1u, true, texture.aspectMask);
          info.sampler = texture.sampler;
        }

        break;
      }

      case false: {
        texture.extraViews.resize(texture.layerCount * texture.levelCount);

        for (uint8_t layerIndex = 0; layerIndex < texture.layerCount;  ++layerIndex) {
          for (uint8_t mipIndex = 0; mipIndex < texture.levelCount; ++mipIndex) {
            VkDescriptorImageInfo& info = texture.extraViews[mipIndex + texture.levelCount * layerIndex];

            info.imageView =
              core::renderer.createImageView(texture.image, texture.imageFormat, layerIndex, 1u, mipIndex, 1u, false, texture.aspectMask);
            info.sampler = texture.sampler;
          }
        }

        break;
      }
    }
  }

  if (!texture.view) {
    RE_LOG(Error, "Failed to create view for texture \"%s\"", name.c_str());
    return RE_ERROR;
  }

  return RE_OK;
}

TResult RTexture::setSampler(
    RSamplerInfo* pSamplerInfo) {
  if (!pSamplerInfo) {
    RE_LOG(Warning, "Sampler info is missing, using the default linear / repeat sampler.");
    texture.sampler = core::renderer.getSampler(nullptr);
  }

  // adapt sampler info to cubemap
  if (isCubemap && texture.layerCount == 6) {
    pSamplerInfo->addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  }

  texture.sampler = core::renderer.getSampler(pSamplerInfo);

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

  texture.imageInfo.imageLayout = texture.imageLayout;
  texture.imageInfo.imageView = texture.view;
  texture.imageInfo.sampler = texture.sampler;

  return RE_OK;
}

RTexture::~RTexture() {
  VkDevice device = core::renderer.logicalDevice.device;

  vkDestroyImageView(device, texture.view, nullptr);

  for (auto& mipView : texture.extraViews) {
    vkDestroyImageView(device, mipView.imageView, nullptr);
  }

  for (auto& faceView : texture.cubemapFaceViews) {
    vkDestroyImageView(device, faceView, nullptr);
  }

  vkDestroySampler(device, texture.sampler, nullptr);

  if (isKTX) {
    ktxVulkanTexture_Destruct(&texture, device, nullptr);
    return;
  }

  vmaDestroyImage(core::renderer.memAlloc, texture.image, allocation);
}