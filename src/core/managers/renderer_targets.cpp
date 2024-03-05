#include "pch.h"
#include "util/math.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::createImageTargets() {
  RTexture* pNewTexture = nullptr;
  RTextureInfo textureInfo{};

  // front target texture used as a source for environment cubemap sides
  std::string rtName = RTGT_ENVSRC;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 1u;
  textureInfo.isCubemap = false;
  textureInfo.width = core::vulkan::envFilterExtent;
  textureInfo.height = textureInfo.width;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // ENVIRONMENT MAPS

  // Subresource range used to "walk" environment maps rendering pass
  environment.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  environment.subresourceRange.baseArrayLayer = 0u;
  environment.subresourceRange.layerCount = 1u;
  environment.subresourceRange.baseMipLevel = 0u;
  environment.subresourceRange.levelCount = 1u;

  rtName = RTGT_ENV;
  uint32_t dimension = core::vulkan::envFilterExtent;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.layerCount = 6u;
  textureInfo.mipLevels = 1u;
  textureInfo.isCubemap = true;
  textureInfo.cubemapFaceViews = true;
  textureInfo.width = dimension;
  textureInfo.height = textureInfo.width;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.extraViews = true;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

  environment.pTargetCubemap = pNewTexture;

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  rtName = RTGT_ENVFILTER;
  dimension = core::vulkan::envFilterExtent;

  textureInfo.name = rtName;
  textureInfo.mipLevels = math::getMipLevels(dimension);
  textureInfo.cubemapFaceViews = false;
  textureInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  rtName = RTGT_ENVIRRAD;
  dimension = core::vulkan::envIrradianceExtent;

  textureInfo.name = rtName;
  textureInfo.width = dimension;
  textureInfo.height = textureInfo.width;
  textureInfo.mipLevels = math::getMipLevels(dimension);

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for BRDF LUT generation
  rtName = RTGT_BRDFMAP;

  textureInfo.name = rtName;
  textureInfo.width = core::vulkan::LUTExtent;
  textureInfo.height = textureInfo.width;
  textureInfo.format = core::vulkan::formatLUT;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 1;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_GENERAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_STORAGE_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for exposure calculation
  rtName = RTGT_EXPOSUREMAP;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = core::vulkan::envIrradianceExtent;
  textureInfo.height = core::vulkan::envIrradianceExtent;
  textureInfo.format = VK_FORMAT_R32_SFLOAT;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = math::getMipLevels(core::vulkan::envIrradianceExtent >> 4);
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // target for post process downsampling
  rtName = RTGT_PPBLOOM;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = config::renderWidth / 2;
  textureInfo.height = config::renderHeight / 2;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 6u;     // A small number of mip maps should be enough for post processing
  textureInfo.extraViews = true;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  textureInfo.samplerInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for TAA, stores history
  rtName = RTGT_PREVFRAME;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = config::renderWidth;
  textureInfo.height = config::renderHeight;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 1u;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT
    | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  textureInfo.samplerInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for TAA, stores final PBR + TAA history and velocity
  rtName = RTGT_PPTAA;

  textureInfo.name = rtName;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for transparency linked list nodes
  rtName = RTGT_ABUFFER;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = config::renderWidth;
  textureInfo.height = config::renderHeight;
  textureInfo.format = VK_FORMAT_R32_UINT;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 1u;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_GENERAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

  scene.pTransparencyStorageTexture = pNewTexture;

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for combining PBR output and A-Buffer
  rtName = RTGT_APBR;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = config::renderWidth;
  textureInfo.height = config::renderHeight;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 1u;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  textureInfo.samplerInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for storing ambient occlusion results
  rtName = RTGT_PPAO;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = config::renderWidth;
  textureInfo.height = config::renderHeight;
  textureInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 1u;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  textureInfo.samplerInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for storing blur results
  rtName = RTGT_PPBLUR;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = config::renderWidth;
  textureInfo.height = config::renderHeight;
  textureInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 1u;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  textureInfo.samplerInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // Target for ambient occlusion noise vectors
  rtName = RTGT_NOISEMAP;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = 4;
  textureInfo.height = 4;
  textureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 1u;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  textureInfo.samplerInfo.filter = VK_FILTER_NEAREST;
  textureInfo.samplerInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // set swapchain subresource copy region
  swapchain.copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swapchain.copyRegion.srcSubresource.baseArrayLayer = 0;
  swapchain.copyRegion.srcSubresource.layerCount = 1;
  swapchain.copyRegion.srcSubresource.mipLevel = 0;
  swapchain.copyRegion.srcOffset = { 0, 0, 0 };

  swapchain.copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swapchain.copyRegion.dstSubresource.baseArrayLayer = 0;
  swapchain.copyRegion.dstSubresource.layerCount = 1;
  swapchain.copyRegion.dstSubresource.mipLevel = 0;
  swapchain.copyRegion.dstOffset = { 0, 0, 0 };

  swapchain.copyRegion.extent.width = swapchain.imageExtent.width;
  swapchain.copyRegion.extent.height = swapchain.imageExtent.height;
  swapchain.copyRegion.extent.depth = 1;

  return createGBufferRenderTargets();
}

TResult core::MRenderer::createGBufferRenderTargets() {
  std::vector<std::string> targetNames;
  scene.pGBufferTargets.clear();

  targetNames.emplace_back(RTGT_GPOSITION);
  targetNames.emplace_back(RTGT_GDIFFUSE);
  targetNames.emplace_back(RTGT_GNORMAL);
  targetNames.emplace_back(RTGT_GPHYSICAL);
  targetNames.emplace_back(RTGT_GEMISSIVE);
  targetNames.emplace_back(RTGT_VELOCITYMAP);

  for (const auto& targetName : targetNames) {
    core::resources.destroyTexture(targetName.c_str(), true);

    VkFormat imageFormat = core::vulkan::formatHDR16;

    // Manage specific render target formats
    if (targetName == RTGT_GPOSITION) imageFormat = core::vulkan::formatHDR32;
    else if (targetName == RTGT_VELOCITYMAP) imageFormat = VK_FORMAT_R16G16_SFLOAT;

    RTexture* pNewTarget;
    if ((pNewTarget = createFragmentRenderTarget(targetName.c_str(), imageFormat)) == nullptr) {
      return RE_CRITICAL;
    }

    scene.pGBufferTargets.emplace_back(pNewTarget);
  }

  if (!createFragmentRenderTarget(RTGT_GPBR, core::vulkan::formatHDR16)) {
    return RE_CRITICAL;
  }

  return createDepthTargets();
}

TResult core::MRenderer::createDepthTargets() {
#ifndef NDEBUG
  RE_LOG(Log, "Creating deferred PBR depth/stencil target.");
#endif

  if (TResult result = getDepthStencilFormat(VK_FORMAT_D32_SFLOAT_S8_UINT, core::vulkan::formatDepth) != RE_OK) {
    return result;
  }

  // Default depth/stencil texture
  std::string rtName = RTGT_DEPTH;

  RTextureInfo textureInfo{};
  textureInfo.name = rtName;
  textureInfo.layerCount = 1u;
  textureInfo.isCubemap = false;
  //textureInfo.format = core::vulkan::formatDepth;
  textureInfo.format = VK_FORMAT_D32_SFLOAT;
  textureInfo.width = swapchain.imageExtent.width;
  textureInfo.height = swapchain.imageExtent.height;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  //textureInfo.targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_GENERAL;
  textureInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  textureInfo.vmaMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

  // Remove image aspect overrides for depth targets in case general layout causes performance issues
  textureInfo.imageAspectOverride = VK_IMAGE_ASPECT_DEPTH_BIT;

  RTexture* pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

  scene.pDepthTarget = pNewTexture;

#ifndef NDEBUG
  RE_LOG(Log, "Created depth target '%s'.", rtName.c_str());
#endif

#ifndef NDEBUG
  RE_LOG(Log, "Creating shadow depth/stencil target.");
#endif

  if (TResult result =
    getDepthStencilFormat(VK_FORMAT_D32_SFLOAT,
      core::vulkan::formatShadow) != RE_OK) {
    return result;
  }

  rtName = RTGT_SHADOW;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.isCubemap = false;
  textureInfo.format = core::vulkan::formatShadow;
  textureInfo.width = config::shadowResolution;
  textureInfo.height = config::shadowResolution;
  textureInfo.layerCount = config::shadowCascades;
  textureInfo.extraViews = true;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  //textureInfo.targetLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_GENERAL;
  textureInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  textureInfo.vmaMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

  textureInfo.samplerInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  textureInfo.imageAspectOverride = VK_IMAGE_ASPECT_DEPTH_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created depth target '%s'.", rtName.c_str());
#endif

  // Target for storing depth to do occlusion culling
  rtName = RTGT_PREVDEPTH;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = config::renderWidth;
  textureInfo.height = config::renderHeight;
  textureInfo.format = VK_FORMAT_R32_SFLOAT;
  //textureInfo.format = VK_FORMAT_D32_SFLOAT;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = math::getMipLevels(config::renderHeight);
  //textureInfo.targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_GENERAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

  // Override with the same image aspect flags as the depth image source
  //textureInfo.imageAspectOverride = core::resources.getTexture(RTGT_DEPTH)->texture.aspectMask;

  textureInfo.samplerInfo.filter = VK_FILTER_LINEAR;
  textureInfo.samplerInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  scene.pPreviousDepthTargets.resize(MAX_FRAMES_IN_FLIGHT);
  for (uint8_t prevDepthIndex = 0; prevDepthIndex < MAX_FRAMES_IN_FLIGHT; ++prevDepthIndex) {
    pNewTexture = core::resources.createTexture(&textureInfo);

    if (!pNewTexture) {
      RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
      return RE_CRITICAL;
    }

    scene.pPreviousDepthTargets[prevDepthIndex] = pNewTexture;

#ifndef NDEBUG
    RE_LOG(Log, "Created depth target '%s'.", rtName.c_str());
#endif
  }

  return RE_OK;
}