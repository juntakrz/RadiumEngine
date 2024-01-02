#include "pch.h"
#include "vk_mem_alloc.h"
#include "util/math.h"
#include "core/core.h"
#include "core/managers/debug.h"
#include "core/managers/window.h"
#include "core/managers/actors.h"
#include "core/managers/world.h"
#include "core/managers/renderer.h"

core::MRenderer::MRenderer() {
  // currently these singletons are passed by reference,
  // so initial log messages have to be here
  RE_LOG(Log, "Radium Engine");
  RE_LOG(Log, "-------------\n");
  RE_LOG(Log, "Initializing engine core...");
  RE_LOG(Log, "Creating graphics manager.");
};

TResult core::MRenderer::createInstance() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = config::appTitle;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = config::engineTitle;
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = core::vulkan::APIversion;

  return createInstance(&appInfo);
}

TResult core::MRenderer::createInstance(VkApplicationInfo* appInfo) {
  RE_LOG(Log, "Creating Vulkan instance.");
  
  if (bRequireValidationLayers) {
    RE_CHECK(checkInstanceValidationLayers());
  }

  std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();

  VkInstanceCreateInfo instCreateInfo{};
  instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instCreateInfo.pApplicationInfo = appInfo;
  instCreateInfo.enabledExtensionCount =
    static_cast<uint32_t>(requiredExtensions.size());
  instCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

  if (!bRequireValidationLayers) {
    instCreateInfo.enabledLayerCount = 0;
    instCreateInfo.pNext = nullptr;
  }
  else {
    instCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(debug::validationLayers.size());
    instCreateInfo.ppEnabledLayerNames = debug::validationLayers.data();
    instCreateInfo.pNext =
      (VkDebugUtilsMessengerCreateInfoEXT*)MDebug::get().info();
  }

  VkResult instCreateResult =
    vkCreateInstance(&instCreateInfo, nullptr, &APIInstance);

  if (instCreateResult != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create Vulkan instance, result code: %d.", instCreateResult);
    return RE_CRITICAL;
  }

  return RE_OK;
}

TResult core::MRenderer::destroyInstance() {
  RE_LOG(Log, "Destroying Vulkan instance.");
  vkDestroyInstance(APIInstance, nullptr);
  return RE_OK;
}

TResult core::MRenderer::createMemAlloc() {
  RE_LOG(Log, "initializing Vulkan memory allocator.");

  VmaAllocatorCreateInfo allocCreateInfo{};
  allocCreateInfo.instance = APIInstance;
  allocCreateInfo.physicalDevice = physicalDevice.device;
  allocCreateInfo.device = logicalDevice.device;
  allocCreateInfo.vulkanApiVersion = core::vulkan::APIversion;

  if (vmaCreateAllocator(&allocCreateInfo, &memAlloc) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create Vulkan memory allocator.");
    return RE_CRITICAL;
  };

  return RE_OK;
}

void core::MRenderer::destroyMemAlloc() {
  RE_LOG(Log, "Destroying Vulkan memory allocator.");
  vmaDestroyAllocator(memAlloc);
}

TResult core::MRenderer::createSurface() {
  RE_LOG(Log, "Creating rendering surface.");

  if (glfwCreateWindowSurface(APIInstance, core::window.getWindow(), nullptr,
                              &surface) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create rendering surface.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroySurface() {
  RE_LOG(Log, "Destroying drawing surface.");
  vkDestroySurfaceKHR(APIInstance, surface, nullptr);
}

TResult core::MRenderer::createSceneBuffers() {
  // set dynamic uniform buffer block sizes
  config::scene::cameraBlockSize =
      static_cast<uint32_t>(util::getVulkanAlignedSize(
          sizeof(RSceneUBO), core::vulkan::minBufferAlignment));
  config::scene::nodeBlockSize =
      static_cast<uint32_t>(util::getVulkanAlignedSize(
          sizeof(glm::mat4) + sizeof(float), core::vulkan::minBufferAlignment));
  config::scene::skinBlockSize =
      static_cast<uint32_t>(util::getVulkanAlignedSize(
          sizeof(glm::mat4) * RE_MAXJOINTS, core::vulkan::minBufferAlignment));

  RE_LOG(Log, "Allocating scene buffer for %d vertices.",
         config::scene::vertexBudget);
  createBuffer(EBufferMode::DGPU_VERTEX, config::scene::getVertexBufferSize(),
               scene.vertexBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d indices.",
         config::scene::indexBudget);
  createBuffer(EBufferMode::DGPU_INDEX, config::scene::getIndexBufferSize(),
               scene.indexBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d entities with transformation.",
         config::scene::entityBudget);
  createBuffer(EBufferMode::CPU_UNIFORM,
               config::scene::getRootTransformBufferSize(),
               scene.rootTransformBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d unique nodes with transformation.",
         config::scene::nodeBudget);
  createBuffer(EBufferMode::CPU_UNIFORM,
               config::scene::getNodeTransformBufferSize(),
               scene.nodeTransformBuffer, nullptr);

  RE_LOG(Log,
         "Allocating scene buffer for %d unique skins.",
         config::scene::entityBudget);
  createBuffer(EBufferMode::CPU_UNIFORM,
               config::scene::getSkinTransformBufferSize(),
               scene.skinTransformBuffer, nullptr);

  return RE_OK;
}

void core::MRenderer::destroySceneBuffers() {
  RE_LOG(Log, "Destroying scene buffers.");
  vmaDestroyBuffer(memAlloc, scene.vertexBuffer.buffer,
                   scene.vertexBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.indexBuffer.buffer,
                   scene.indexBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.rootTransformBuffer.buffer,
                   scene.rootTransformBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.nodeTransformBuffer.buffer,
                   scene.nodeTransformBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.skinTransformBuffer.buffer,
                   scene.skinTransformBuffer.allocation);
}

core::MRenderer::RSceneBuffers* core::MRenderer::getSceneBuffers() {
  return &scene;
}

TResult core::MRenderer::createDescriptorPool() {
  RE_LOG(Log, "Creating descriptor pool.");

  // the number of descriptors in the given pool per set/layout type
  std::vector<VkDescriptorPoolSize> poolSizes;
  poolSizes.resize(3);

  // model view projection matrix
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  // materials and textures
  // TODO: rewrite so that descriptorCounts are calculated by objects/textures
  // using map data e.g. max textures that are going to be loaded
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = 2000 * MAX_FRAMES_IN_FLIGHT;

  // model nodes
  // TODO: rewrite so that descriptorCounts are calculated by objects/textures
  // using map data e.g. max unique model nodes that are going to be loaded
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[2].descriptorCount = 2000 * MAX_FRAMES_IN_FLIGHT;

  uint32_t maxSets = 0;
  for (uint8_t i = 0; i < poolSizes.size(); ++i) {
    maxSets += poolSizes[i].descriptorCount;
  }
  maxSets += 100;  // descriptor set headroom

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = maxSets;
  poolInfo.flags = 0;

  if (vkCreateDescriptorPool(logicalDevice.device, &poolInfo, nullptr,
                             &system.descriptorPool) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create descriptor pool.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroyDescriptorPool() {
  RE_LOG(Log, "Destroying descriptor pool.");
  vkDestroyDescriptorPool(logicalDevice.device, system.descriptorPool, nullptr);
}

TResult core::MRenderer::createUniformBuffers() {
  // TODO: dynamic uniform buffers with an offset tied to frame in flight
  view.modelViewProjectionBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  lighting.buffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkDeviceSize uboMVPSize =
      config::scene::cameraBlockSize * config::scene::getMaxCameraCount();
  VkDeviceSize uboLightingSize = sizeof(RLightingUBO);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    //createBuffer(EBufferMode::CPU_UNIFORM, uboMVPSize,
    //             view.modelViewProjectionBuffers[i], getSceneUBO());
    createBuffer(EBufferMode::CPU_UNIFORM, uboMVPSize,
                 view.modelViewProjectionBuffers[i], nullptr);
    createBuffer(EBufferMode::CPU_UNIFORM, uboLightingSize, lighting.buffers[i],
                 &lighting.data);
  }

  // create environment buffer
  core::vulkan::minBufferAlignment =
      physicalDevice.properties.limits.minUniformBufferOffsetAlignment;
  VkDeviceSize alignedSize = util::getVulkanAlignedSize(
      sizeof(REnvironmentUBO), core::vulkan::minBufferAlignment);
  VkDeviceSize bufferSize = alignedSize * 6;

  environment.transformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  environment.transformOffset = alignedSize;

  for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j) {
    createBuffer(EBufferMode::CPU_UNIFORM, bufferSize,
                 environment.transformBuffers[j], nullptr);
  }

  // environment transformations need to be set only once
  setEnvironmentUBO();

  return RE_OK;
}

void core::MRenderer::destroyUniformBuffers() {
  for (auto& it : view.modelViewProjectionBuffers) {
    vmaDestroyBuffer(memAlloc, it.buffer, it.allocation);
  }

  for (auto& it : lighting.buffers) {
    vmaDestroyBuffer(memAlloc, it.buffer, it.allocation);
  }

  for (auto& it : environment.transformBuffers) {
    vmaDestroyBuffer(memAlloc, it.buffer, it.allocation);
  }
}

TResult core::MRenderer::createImageTargets() {
  RTexture* pNewTexture = nullptr;
  RTextureInfo textureInfo{};
  environment.destinationRanges.resize(2);
  environment.destinationTextures.resize(2);

  // front target texture used as a source for environment cubemap sides
  std::string rtName = RTGT_ENVSRC;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.asCubemap = false;
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

  // set subresource range for future copying from this render target
  environment.sourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  environment.sourceRange.baseArrayLayer = 0;
  environment.sourceRange.layerCount = pNewTexture->texture.layerCount;
  environment.sourceRange.baseMipLevel = 0;
  environment.sourceRange.levelCount = pNewTexture->texture.levelCount;

  environment.copyRegion.srcSubresource.aspectMask =
      VK_IMAGE_ASPECT_COLOR_BIT;
  environment.copyRegion.srcSubresource.baseArrayLayer = 0;
  environment.copyRegion.srcSubresource.layerCount = 1;
  environment.copyRegion.srcSubresource.mipLevel = 0;
  environment.copyRegion.srcOffset = {0, 0, 0};

  environment.copyRegion.dstSubresource.aspectMask =
      VK_IMAGE_ASPECT_COLOR_BIT;
  environment.copyRegion.dstSubresource.layerCount = 1;
  environment.copyRegion.dstOffset = {0, 0, 0};
  environment.copyRegion.extent.depth = pNewTexture->texture.depth;

  environment.pSourceTexture = pNewTexture;

  // target for BRDF LUT generation
  rtName = RTGT_LUTMAP;

  textureInfo.name = rtName;
  textureInfo.width = core::vulkan::LUTExtent;
  textureInfo.height = textureInfo.width;
  textureInfo.format = core::vulkan::formatLUT;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.usageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // default cubemap texture as a copy target
  rtName = RTGT_ENVFILTER;
  uint32_t dimension = core::vulkan::envFilterExtent;
  uint32_t mipLevels = static_cast<uint32_t>(floor(log2(dimension))) + 1;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.asCubemap = true;
  textureInfo.width = dimension;
  textureInfo.height = textureInfo.width;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.mipLevels = mipLevels;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  environment.destinationRanges[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  environment.destinationRanges[0].baseArrayLayer = 0;
  environment.destinationRanges[0].layerCount = pNewTexture->texture.layerCount;
  environment.destinationRanges[0].baseMipLevel = 0;
  environment.destinationRanges[0].levelCount = pNewTexture->texture.levelCount;

  environment.destinationTextures[0] = pNewTexture;

  rtName = RTGT_ENVIRRAD;
  dimension = core::vulkan::envIrradianceExtent;
  mipLevels = static_cast<uint32_t>(floor(log2(dimension))) + 1;

  textureInfo.name = rtName;
  textureInfo.width = dimension;
  textureInfo.height = textureInfo.width;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.mipLevels = mipLevels;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  environment.destinationRanges[1].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  environment.destinationRanges[1].baseArrayLayer = 0;
  environment.destinationRanges[1].layerCount = pNewTexture->texture.layerCount;
  environment.destinationRanges[1].baseMipLevel = 0;
  environment.destinationRanges[1].levelCount = pNewTexture->texture.levelCount;

  environment.destinationTextures[1] = pNewTexture;

  rtName = RTGT_COMPUTE;
  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = swapchain.imageExtent.width;
  textureInfo.height = swapchain.imageExtent.height;
  textureInfo.asCubemap = false;
  textureInfo.format = core::vulkan::formatHDR32;
  textureInfo.layerCount = 1;
  textureInfo.mipLevels = 1;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_GENERAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_STORAGE_BIT |
                           VK_IMAGE_USAGE_SAMPLED_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

  compute.pComputeImageTarget = pNewTexture;

#ifndef NDEBUG
  RE_LOG(Log, "Created image target '%s'.", rtName.c_str());
#endif

  // set environment push block defaults
  environment.envPushBlock.samples = 32u;
  environment.envPushBlock.roughness = 0.0f;

  // set swapchain subresource copy region
  swapchain.copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swapchain.copyRegion.srcSubresource.baseArrayLayer = 0;
  swapchain.copyRegion.srcSubresource.layerCount = 1;
  swapchain.copyRegion.srcSubresource.mipLevel = 0;
  swapchain.copyRegion.srcOffset = {0, 0, 0};

  swapchain.copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swapchain.copyRegion.dstSubresource.baseArrayLayer = 0;
  swapchain.copyRegion.dstSubresource.layerCount = 1;
  swapchain.copyRegion.dstSubresource.mipLevel = 0;
  swapchain.copyRegion.dstOffset = {0, 0, 0};

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

  for (const auto& targetName : targetNames) {
    core::resources.destroyTexture(targetName.c_str(), true);

    RTexture* pNewTarget;
    if ((pNewTarget = createFragmentRenderTarget(targetName.c_str())) ==
        nullptr) {
      return RE_CRITICAL;
    }

    scene.pGBufferTargets.emplace_back(pNewTarget);
  }

  if (!createFragmentRenderTarget(RTGT_GPBR)) {
    return RE_CRITICAL;
  }

  return createDepthTargets();
}

TResult core::MRenderer::createDepthTargets() {
#ifndef NDEBUG
  RE_LOG(Log, "Creating deferred PBR depth/stencil target.");
#endif

  if (TResult result =
          getDepthStencilFormat(VK_FORMAT_D32_SFLOAT_S8_UINT,
                                core::vulkan::formatDepth) != RE_OK) {
    return result;
  }

  // default depth/stencil texture
  std::string rtName = RTGT_DEPTH;

  RTextureInfo textureInfo{};
  textureInfo.name = rtName;
  textureInfo.asCubemap = false;
  textureInfo.format = core::vulkan::formatDepth;
  textureInfo.width = swapchain.imageExtent.width;
  textureInfo.height = swapchain.imageExtent.height;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  textureInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  textureInfo.vmaMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

  RTexture* pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

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
  textureInfo.asCubemap = false;
  textureInfo.format = core::vulkan::formatShadow;
  textureInfo.width = config::shadowResolution;
  textureInfo.height = config::shadowResolution;
  textureInfo.layerCount = config::shadowCascades;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  textureInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  textureInfo.vmaMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Created depth target '%s'.", rtName.c_str());
#endif

  return RE_OK;
}

TResult core::MRenderer::createRendererDefaults() {
  // create default images
  TResult chkResult = createImageTargets();

  if (chkResult != RE_OK) {
    return chkResult;
  }

  // create default camera
  RCameraInfo cameraInfo{};
  cameraInfo.FOV = config::FOV;
  cameraInfo.aspectRatio = 1.0f;
  cameraInfo.nearZ = RE_NEARZ;
  cameraInfo.farZ = config::viewDistance;

  ACamera* pCamera = core::actors.createCamera(RCAM_MAIN, &cameraInfo);

  if (!pCamera) {
    return RE_CRITICAL;
  }

  setCamera(pCamera);

  return RE_OK;
}

TResult core::MRenderer::createCoreCommandPools() {
  RE_LOG(Log, "Creating command pool.");

  VkCommandPoolCreateInfo cmdPoolRenderInfo{};
  cmdPoolRenderInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolRenderInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cmdPoolRenderInfo.queueFamilyIndex =
      physicalDevice.queueFamilyIndices.graphics[0];

  if (vkCreateCommandPool(logicalDevice.device, &cmdPoolRenderInfo, nullptr,
                          &command.poolGraphics) != VK_SUCCESS) {
    RE_LOG(Critical,
           "failed to create command pool for graphics queue family.");

    return RE_CRITICAL;
  }

  cmdPoolRenderInfo.queueFamilyIndex =
      physicalDevice.queueFamilyIndices.transfer[0];

  if (vkCreateCommandPool(logicalDevice.device, &cmdPoolRenderInfo, nullptr,
                          &command.poolTransfer) != VK_SUCCESS) {
    RE_LOG(Critical,
           "failed to create command pool for transfer queue family.");

    return RE_CRITICAL;
  }

  cmdPoolRenderInfo.queueFamilyIndex =
      physicalDevice.queueFamilyIndices.compute[0];

  if (vkCreateCommandPool(logicalDevice.device, &cmdPoolRenderInfo, nullptr,
                          &command.poolCompute) != VK_SUCCESS) {
    RE_LOG(Critical, "failed to create command pool for compute queue family.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroyCoreCommandPools() {
  RE_LOG(Log, "Destroying command pools.");
  vkDestroyCommandPool(logicalDevice.device, command.poolGraphics, nullptr);
  vkDestroyCommandPool(logicalDevice.device, command.poolTransfer, nullptr);
  vkDestroyCommandPool(logicalDevice.device, command.poolCompute, nullptr);
}

TResult core::MRenderer::createCoreCommandBuffers() {
  RE_LOG(Log, "Creating graphics command buffers for %d frames.",
         MAX_FRAMES_IN_FLIGHT);

  command.buffersGraphics.resize(MAX_FRAMES_IN_FLIGHT);

  for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    command.buffersGraphics[i] = createCommandBuffer(
        ECmdType::Graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

    if (command.buffersGraphics[i] == nullptr) {
      RE_LOG(Critical, "Failed to allocate graphics command buffers.");
      return RE_CRITICAL;
    }
  }

  RE_LOG(Log, "Creating %d transfer command buffers.", MAX_TRANSFER_BUFFERS);

  command.buffersTransfer.resize(MAX_TRANSFER_BUFFERS);

  for (uint8_t j = 0; j < MAX_TRANSFER_BUFFERS; ++j) {
    command.buffersTransfer[j] = createCommandBuffer(
        ECmdType::Transfer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

    if (command.buffersTransfer[j] == nullptr) {
      RE_LOG(Critical, "Failed to allocate transfer command buffers.");
      return RE_CRITICAL;
    }
  }

  return RE_OK;
}

void core::MRenderer::destroyCoreCommandBuffers() {
  RE_LOG(Log, "Freeing %d graphics command buffers.",
         command.buffersGraphics.size());
  vkFreeCommandBuffers(logicalDevice.device, command.poolGraphics,
                       static_cast<uint32_t>(command.buffersGraphics.size()),
                       command.buffersGraphics.data());

  RE_LOG(Log, "Freeing %d compute command buffers.",
         command.buffersCompute.size());
  vkFreeCommandBuffers(logicalDevice.device, command.poolCompute,
                       static_cast<uint32_t>(command.buffersCompute.size()),
                       command.buffersGraphics.data());

  RE_LOG(Log, "Freeing %d transfer command buffer.",
         command.buffersTransfer.size());
  vkFreeCommandBuffers(logicalDevice.device, command.poolTransfer,
                       static_cast<uint32_t>(command.buffersTransfer.size()),
                       command.buffersTransfer.data());
}

TResult core::MRenderer::createSyncObjects() {
  RE_LOG(Log, "Creating sychronization objects for %d frames.",
         MAX_FRAMES_IN_FLIGHT);

  sync.semImgAvailable.resize(MAX_FRAMES_IN_FLIGHT);
  sync.semRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
  sync.fenceInFlight.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semInfo{};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenInfo{};
  fenInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // signaled to skip waiting for
                                                 // it on the first frame

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(logicalDevice.device, &semInfo, nullptr,
                          &sync.semImgAvailable[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "failed to create 'image available' semaphore.");

      return RE_CRITICAL;
    }

    if (vkCreateSemaphore(logicalDevice.device, &semInfo, nullptr,
                          &sync.semRenderFinished[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "failed to create 'render finished' semaphore.");

      return RE_CRITICAL;
    }

    if (vkCreateFence(logicalDevice.device, &fenInfo, nullptr,
                      &sync.fenceInFlight[i])) {
      RE_LOG(Critical, "failed to create 'in flight' fence.");

      return RE_CRITICAL;
    }
  }

  // create continuously running threads
  RE_LOG(Log, "Creating entity update thread.");
  sync.asyncUpdateEntities.bindFunction(this, &MRenderer::updateBoundEntities);
  sync.asyncUpdateEntities.start();

  return RE_OK;
}

void core::MRenderer::destroySyncObjects() {
  RE_LOG(Log, "Destroying synchronization objects.");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(logicalDevice.device, sync.semImgAvailable[i], nullptr);
    vkDestroySemaphore(logicalDevice.device, sync.semRenderFinished[i],
                       nullptr);

    vkDestroyFence(logicalDevice.device, sync.fenceInFlight[i], nullptr);
  }

  RE_LOG(Log, "Stopping entity update thread.");
  sync.asyncUpdateEntities.stop();
}

void core::MRenderer::setEnvironmentUBO() {
  // prepare transformation matrices
  std::array<glm::mat4, 6> transformArray;
  size_t projectionOffset = sizeof(glm::mat4);

  glm::mat4 perspective =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 10.0f);

  transformArray[0] =
      glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));   // X+
  transformArray[1] =
      glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));    // X-
  transformArray[2] =
      glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));    // Y+
  transformArray[3] =
      glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));   // Y-
  transformArray[4] = glm::mat4(1.0f);                                  // Z+
  transformArray[5] =
      glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));   // Z-

  for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    uint8_t* memAddress = static_cast<uint8_t*>(
        environment.transformBuffers[i].allocInfo.pMappedData);

    for (uint8_t j = 0; j < transformArray.size(); ++j) {
      memcpy(memAddress, &transformArray[j], sizeof(glm::mat4));
      memcpy(memAddress + projectionOffset, &perspective, sizeof(glm::mat4));
      memAddress += environment.transformOffset;
    }
  }
}

void core::MRenderer::updateSceneUBO(uint32_t currentImage) {
  view.worldViewProjectionData.view = view.pActiveCamera->getView();
  view.worldViewProjectionData.projection = view.pActiveCamera->getProjection();
  view.worldViewProjectionData.cameraPosition =
      view.pActiveCamera->getLocation();

  uint8_t* pSceneUBO =
      static_cast<uint8_t*>(
          view.modelViewProjectionBuffers[currentImage].allocInfo.pMappedData) +
      config::scene::cameraBlockSize * view.pActiveCamera->getViewBufferIndex();

  memcpy(pSceneUBO, &view.worldViewProjectionData, sizeof(RSceneUBO));
}

void core::MRenderer::waitForSystemIdle() {
  vkQueueWaitIdle(logicalDevice.queues.graphics);
  vkQueueWaitIdle(logicalDevice.queues.compute);
  vkQueueWaitIdle(logicalDevice.queues.present);
  vkDeviceWaitIdle(logicalDevice.device);
}

TResult core::MRenderer::initialize() {
  TResult chkResult = RE_OK;

  chkResult = core::renderer.createInstance();

  // debug manager setup
  if (chkResult <= RE_ERRORLIMIT)
    chkResult = MDebug::get().create(core::renderer.APIInstance);

  if (chkResult <= RE_ERRORLIMIT) chkResult = createSurface();

  if (chkResult <= RE_ERRORLIMIT) chkResult = enumPhysicalDevices();
  if (chkResult <= RE_ERRORLIMIT) chkResult = initPhysicalDevice();
  if (chkResult <= RE_ERRORLIMIT) chkResult = initLogicalDevice();

  if (chkResult <= RE_ERRORLIMIT) chkResult = createMemAlloc();

  if (chkResult <= RE_ERRORLIMIT)
    chkResult =
    initSwapChain(core::vulkan::formatLDR, core::vulkan::colorSpace,
                      core::vulkan::presentMode);
  updateAspectRatio();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCoreCommandPools();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCoreCommandBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createSceneBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createRendererDefaults();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSetLayouts();

  if (chkResult <= RE_ERRORLIMIT) chkResult = createRenderPasses();           // A
  if (chkResult <= RE_ERRORLIMIT) chkResult = createGraphicsPipelines();      // B
  if (chkResult <= RE_ERRORLIMIT) chkResult = createComputePipelines();       // C
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDefaultFramebuffers();    // D
  if (chkResult <= RE_ERRORLIMIT) chkResult = configureRenderPasses();        // tie A, B, C, D together

  if (chkResult <= RE_ERRORLIMIT) chkResult = createSyncObjects();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createUniformBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorPool();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSets();

  return chkResult;
}

void core::MRenderer::deinitialize() {
  waitForSystemIdle();

  destroySwapChain();
  destroySyncObjects();
  destroyCoreCommandBuffers();
  destroyCoreCommandPools();
  destroyComputePipelines();
  destroyGraphicsPipelines();
  destroyRenderPasses();
  destroySceneBuffers();
  destroySurface();
  core::actors.destroyAllPawns();
  core::world.destroyAllModels();
  core::resources.destroyAllTextures();
  destroyDescriptorPool();
  destroyUniformBuffers();
  destroyMemAlloc();
  if(bRequireValidationLayers) MDebug::get().destroy(APIInstance);
  destroyLogicalDevice();
  destroyInstance();
}

const VkDescriptorSetLayout core::MRenderer::getDescriptorSetLayout(EDescriptorSetLayout type) const {
  return system.descriptorSetLayouts.at(type);
}

const VkDescriptorPool core::MRenderer::getDescriptorPool() {
  return system.descriptorPool;
}

const VkDescriptorSet core::MRenderer::getDescriptorSet(
    uint32_t frameInFlight) {
  return frameInFlight == -1 ? system.descriptorSets[renderView.frameInFlight]
                             : system.descriptorSets[frameInFlight];
}

RSceneUBO* core::MRenderer::getSceneUBO() {
  return &view.worldViewProjectionData;
}

void core::MRenderer::queueLightingUBOUpdate() {
  lighting.tracking.bufferUpdatesRemaining = MAX_FRAMES_IN_FLIGHT;
  lighting.tracking.dataRequiresUpdate = true;
}

void core::MRenderer::updateLightingUBO(const int32_t frameIndex) {
  if (lighting.tracking.bufferUpdatesRemaining < 1) return;

  if (lighting.tracking.dataRequiresUpdate) {
    core::actors.updateLightingUBO(&lighting.data);
    lighting.tracking.dataRequiresUpdate = false;
  }

  memcpy(lighting.buffers[frameIndex].allocInfo.pMappedData, &lighting.data,
         sizeof(RLightingUBO));
  lighting.tracking.bufferUpdatesRemaining--;
}
