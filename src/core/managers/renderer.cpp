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
  
  if (requireValidationLayers) {
    RE_CHECK(checkInstanceValidationLayers());
  }

  VkValidationFeaturesEXT validationFeatures{};
  validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;

  std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();

  VkInstanceCreateInfo instCreateInfo{};
  instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instCreateInfo.pApplicationInfo = appInfo;
  instCreateInfo.enabledExtensionCount =
    static_cast<uint32_t>(requiredExtensions.size());
  instCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

  if (!requireValidationLayers) {
    instCreateInfo.enabledLayerCount = 0;
    instCreateInfo.pNext = nullptr;
  }
  else {
    instCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(debug::validationLayers.size());
    instCreateInfo.ppEnabledLayerNames = debug::validationLayers.data();

    if (!enableGPUAssistedValidation) {
      instCreateInfo.pNext =
        (VkDebugUtilsMessengerCreateInfoEXT*)MDebug::get().info();
    }
    else {
      bool GPUValidationAvailable;
      getInstanceExtensions("VK_LAYER_KHRONOS_validation", VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME, &GPUValidationAvailable);

      if (GPUValidationAvailable) {
        const uint32_t enabledCount = 2;

        VkValidationFeatureEnableEXT enabledFeatures[enabledCount] = {
          VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
          VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        };

        validationFeatures.pEnabledValidationFeatures = enabledFeatures;
        validationFeatures.enabledValidationFeatureCount = enabledCount;
        validationFeatures.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)MDebug::get().info();

        instCreateInfo.pNext = &validationFeatures;

        RE_LOG(Warning, "Enabling GPU assisted validation layer.");
      }
    }
  }

  VkResult instCreateResult = vkCreateInstance(&instCreateInfo, nullptr, &APIInstance);

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
  allocCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

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
          sizeof(RSceneUBO), core::vulkan::minUniformBufferAlignment));
  config::scene::nodeBlockSize =
      static_cast<uint32_t>(util::getVulkanAlignedSize(
          sizeof(glm::mat4) + sizeof(float), core::vulkan::minUniformBufferAlignment));
  config::scene::skinBlockSize =
      static_cast<uint32_t>(util::getVulkanAlignedSize(
          sizeof(glm::mat4) * RE_MAXJOINTS, core::vulkan::minUniformBufferAlignment));

  RE_LOG(Log, "Allocating scene storage vertex buffer for %d vertices.",
    config::scene::vertexBudget);
  createBuffer(EBufferType::DGPU_VERTEX, config::scene::getVertexBufferSize(),
    scene.vertexBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d indices.",
         config::scene::indexBudget);
  createBuffer(EBufferType::DGPU_INDEX, config::scene::getIndexBufferSize(),
               scene.indexBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d entities with transformation.",
         config::scene::entityBudget);
  createBuffer(EBufferType::CPU_UNIFORM,
               config::scene::getRootTransformBufferSize(),
               scene.rootTransformBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d unique nodes with transformation.",
         config::scene::nodeBudget);
  createBuffer(EBufferType::CPU_UNIFORM,
               config::scene::getNodeTransformBufferSize(),
               scene.nodeTransformBuffer, nullptr);

  RE_LOG(Log,
         "Allocating scene buffer for %d unique skins.",
         config::scene::entityBudget);
  createBuffer(EBufferType::CPU_UNIFORM,
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

core::MRenderer::RLightingData* core::MRenderer::getLightingData() {
  return &lighting;
}

VkDescriptorSet core::MRenderer::getMaterialDescriptorSet() {
  return material.descriptorSet;
}

core::MRenderer::RMaterialData* core::MRenderer::getMaterialData() {
  return &material;
}

core::MRenderer::RSceneBuffers* core::MRenderer::getSceneBuffers() {
  return &scene;
}

TResult core::MRenderer::createUniformBuffers() {
  view.modelViewProjectionBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  lighting.buffers.resize(MAX_FRAMES_IN_FLIGHT);

  core::vulkan::minUniformBufferAlignment = physicalDevice.deviceProperties.properties.limits.minUniformBufferOffsetAlignment;
  core::vulkan::descriptorBufferOffsetAlignment = physicalDevice.descriptorBufferProperties.descriptorBufferOffsetAlignment;

  VkDeviceSize uboMVPSize =
      config::scene::cameraBlockSize * config::scene::getMaxCameraCount();
  VkDeviceSize uboLightingSize = sizeof(RLightingUBO);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    createBuffer(EBufferType::CPU_UNIFORM, uboMVPSize,
                 view.modelViewProjectionBuffers[i], nullptr);
    createBuffer(EBufferType::CPU_UNIFORM, uboLightingSize, lighting.buffers[i],
                 &lighting.data);
  }

  return RE_OK;
}

void core::MRenderer::destroyUniformBuffers() {
  for (auto& it : view.modelViewProjectionBuffers) {
    vmaDestroyBuffer(memAlloc, it.buffer, it.allocation);
  }

  for (auto& it : lighting.buffers) {
    vmaDestroyBuffer(memAlloc, it.buffer, it.allocation);
  }
}

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

  // target for BRDF LUT generation
  rtName = RTGT_LUTMAP;

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

  // target for post process downsampling
  rtName = RTGT_PPDOWNSMPL;
  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.width = config::renderWidth / 2;
  textureInfo.height = config::renderHeight / 2;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.isCubemap = false;
  textureInfo.layerCount = 1u;
  textureInfo.mipLevels = 5u;     // A small number of mip maps should be enough for post processing
  textureInfo.extraViews = true;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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

    VkFormat imageFormat = (targetName == RTGT_GPOSITION) ? core::vulkan::formatHDR32 : core::vulkan::formatHDR16;

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

  // default depth/stencil texture
  std::string rtName = RTGT_DEPTH;

  RTextureInfo textureInfo{};
  textureInfo.name = rtName;
  textureInfo.layerCount = 1u;
  textureInfo.isCubemap = false;
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
  textureInfo.isCubemap = false;
  textureInfo.format = core::vulkan::formatShadow;
  textureInfo.width = config::shadowResolution;
  textureInfo.height = config::shadowResolution;
  textureInfo.layerCount = config::shadowCascades;
  textureInfo.extraViews = true;
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

TResult core::MRenderer::setDefaultComputeJobs() {
  // BRDF LUT
  {
    RComputeJobInfo& info = environment.computeJobs.LUT;
    info.jobType = EComputeJob::Image;
    info.pipeline = EComputePipeline::ImageLUT;
    info.width = core::vulkan::LUTExtent / 8;
    info.height = core::vulkan::LUTExtent / 4;
    info.transtionToShaderReadOnly = true;
    info.pImageAttachments = { core::resources.getTexture(RTGT_LUTMAP) };
  }

  // Environment irradiance map
  {
    RComputeJobInfo& info = environment.computeJobs.irradiance;
    info.jobType = EComputeJob::Image;
    info.pipeline = EComputePipeline::ImageEnvIrradiance;
    info.width = core::vulkan::envFilterExtent / 8;
    info.height = core::vulkan::envFilterExtent / 4;
    info.depth = 1;
    info.transtionToShaderReadOnly = true;
    info.useExtraImageViews = true;
    info.useExtraSamplerViews = false;
    info.pImageAttachments = { core::resources.getTexture(RTGT_ENVIRRAD) };
    info.pSamplerAttachments = { core::resources.getTexture(RTGT_ENV) };
    info.intValues.x = info.pImageAttachments[0]->texture.levelCount;
  }

  // Environment prefiltered map
  {
    RComputeJobInfo& info = environment.computeJobs.prefiltered;
    info.jobType = EComputeJob::Image;
    info.pipeline = EComputePipeline::ImageEnvFilter;
    info.width = core::vulkan::envFilterExtent / 8;
    info.height = core::vulkan::envFilterExtent / 4;
    info.depth = 1;
    info.transtionToShaderReadOnly = true;
    info.useExtraImageViews = true;
    info.useExtraSamplerViews = false;
    info.pImageAttachments = { core::resources.getTexture(RTGT_ENVFILTER) };
    info.pSamplerAttachments = { core::resources.getTexture(RTGT_ENV) };
    info.intValues.x = info.pImageAttachments[0]->texture.levelCount;
  }

  return RE_OK;
}

TResult core::MRenderer::setRendererDefaults() {
  // Setup bindless resource budgets
  uint32_t maxAfterBindSamplers = physicalDevice.descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindSamplers;
  maxAfterBindSamplers -= maxAfterBindSamplers / 10;
  config::scene::sampledImageBudget = maxAfterBindSamplers;

  uint32_t maxAfterBindStorageImages = physicalDevice.descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindStorageImages;
  maxAfterBindStorageImages -= maxAfterBindStorageImages / 10;
  config::scene::storageImageBudget = (config::scene::requestedStorageImageBudget > maxAfterBindStorageImages)
    ? maxAfterBindStorageImages : config::scene::requestedStorageImageBudget;

  // Create default images
  TResult chkResult = createImageTargets();

  if (chkResult != RE_OK) {
    return chkResult;
  }

  chkResult = setDefaultComputeJobs();

  if (chkResult != RE_OK) {
    return chkResult;
  }

  // Create default cameras

  // RCAM_ENV
  RCameraInfo cameraInfo{};
  cameraInfo.FOV = 90.0f;
  cameraInfo.aspectRatio = 1.0f;
  cameraInfo.nearZ = RE_NEARZ;
  cameraInfo.farZ = config::viewDistance;

  ACamera* pCamera = core::actors.createCamera(RCAM_ENV, &cameraInfo);

  // Set transformation array for the environment camera
  environment.cameraTransformVectors[0] = glm::vec3(0.0f, glm::radians(90.0f), 0.0f);   // X+
  environment.cameraTransformVectors[1] = glm::vec3(0.0f, glm::radians(-90.0f), 0.0f);  // X-
  environment.cameraTransformVectors[2] = glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f);  // Y+
  environment.cameraTransformVectors[3] = glm::vec3(glm::radians(90.0f), 0.0f, 0.0f);   // Y-
  environment.cameraTransformVectors[4] = glm::vec3(0.0f, 0.0f, 0.0f);                  // Z+
  environment.cameraTransformVectors[5] = glm::vec3(0.0f, glm::radians(180.0f), 0.0f);  // Z-

  // Make environment camera ignore pitch limit
  pCamera->setIgnorePitchLimit(true);

  // RCAM_MAIN
  cameraInfo.FOV = config::FOV;
  pCamera = core::actors.createCamera(RCAM_MAIN, &cameraInfo);

  if (!pCamera) {
    return RE_CRITICAL;
  }

  setCamera(pCamera);

  // Set default lighting UBO data
  lighting.data.prefilteredCubeMipLevels = (float)math::getMipLevels(core::vulkan::envFilterExtent);

  // Set default post processing info
  postprocess.pDownsampleTexture = core::resources.getTexture(RTGT_PPDOWNSMPL);

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

void core::MRenderer::updateSceneUBO(uint32_t currentImage) {
  view.worldViewProjectionData.view = view.pActiveCamera->getView();
  view.worldViewProjectionData.projection = view.pActiveCamera->getProjection();
  view.worldViewProjectionData.cameraPosition = view.pActiveCamera->getLocation();

  uint8_t* pSceneUBO = static_cast<uint8_t*>(view.modelViewProjectionBuffers[currentImage].allocInfo.pMappedData) +
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
    chkResult = initSwapChain(core::vulkan::formatLDR, core::vulkan::colorSpace, core::vulkan::presentMode);
  updateAspectRatio();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCoreCommandPools();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCoreCommandBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createSceneBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = setRendererDefaults();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSetLayouts();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createPipelineLayouts();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createViewports();

  if (chkResult <= RE_ERRORLIMIT) chkResult = createDynamicRenderingPasses();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createComputePipelines();

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
  destroyDynamicRenderingPasses();
  destroySceneBuffers();
  destroySurface();
  core::actors.destroyAllPawns();
  core::world.destroyAllModels();
  core::resources.destroyAllTextures();
  destroyDescriptorPool();
  destroyUniformBuffers();
  destroyMemAlloc();
  if(requireValidationLayers) MDebug::get().destroy(APIInstance);
  destroyLogicalDevice();
  destroyInstance();
}

const VkDescriptorSet core::MRenderer::getSceneDescriptorSet(
    uint32_t frameInFlight) {
  return frameInFlight == -1 ? scene.descriptorSets[renderView.frameInFlight]
                             : scene.descriptorSets[frameInFlight];
}

RSceneUBO* core::MRenderer::getSceneUBO() {
  return &view.worldViewProjectionData;
}

core::MRenderer::REnvironmentData* core::MRenderer::getEnvironmentData() {
  return &environment;
}

void core::MRenderer::updateLightingUBO(const int32_t frameIndex) {
  core::actors.updateLightingUBO(&lighting.data);

  memcpy(lighting.buffers[frameIndex].allocInfo.pMappedData, &lighting.data,
         sizeof(RLightingUBO));
}

core::MRenderer::RPostProcessData* core::MRenderer::getPostProcessingData() {
  return &postprocess;
}

void core::MRenderer::updateAspectRatio() {
  view.cameraSettings.aspectRatio =
    (float)swapchain.imageExtent.width / swapchain.imageExtent.height;
}

void core::MRenderer::setFOV(float FOV) { view.pActiveCamera->setFOV(FOV); }

void core::MRenderer::setViewDistance(float farZ) { view.cameraSettings.farZ; }

void core::MRenderer::setViewDistance(float nearZ, float farZ) {
  view.cameraSettings.nearZ = nearZ;
  view.cameraSettings.farZ = farZ;
}