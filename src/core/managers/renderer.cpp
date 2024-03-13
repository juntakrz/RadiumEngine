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
  scene.sceneBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  lighting.buffers.resize(MAX_FRAMES_IN_FLIGHT);

  core::vulkan::minUniformBufferAlignment = physicalDevice.deviceProperties.properties.limits.minUniformBufferOffsetAlignment;
  core::vulkan::descriptorBufferOffsetAlignment = physicalDevice.descriptorBufferProperties.descriptorBufferOffsetAlignment;

  // set dynamic uniform buffer block sizes
  config::scene::cameraBlockSize =
      static_cast<uint32_t>(util::getVulkanAlignedSize(sizeof(RSceneUBO), core::vulkan::minUniformBufferAlignment));
  config::scene::nodeBlockSize =
      static_cast<uint32_t>(util::getVulkanAlignedSize(sizeof(glm::mat4) * 2 + sizeof(float), core::vulkan::minUniformBufferAlignment));
  config::scene::skinBlockSize =
      static_cast<uint32_t>(util::getVulkanAlignedSize(sizeof(glm::mat4) * RE_MAXJOINTS * 2, core::vulkan::minUniformBufferAlignment));

  RE_LOG(Log, "Allocating scene and lighting uniform buffers");
  VkDeviceSize uboMVPSize =
    config::scene::cameraBlockSize * config::scene::getMaxCameraCount();
  VkDeviceSize uboLightingSize = sizeof(RLightingUBO);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    createBuffer(EBufferType::CPU_UNIFORM, uboMVPSize,
      scene.sceneBuffers[i], nullptr);
    createBuffer(EBufferType::CPU_UNIFORM, uboLightingSize, lighting.buffers[i],
      &lighting.data);
  }

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
  createBuffer(EBufferType::CPU_STORAGE,
               config::scene::getRootTransformBufferSize(),
               scene.modelTransformBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d unique nodes with transformation.",
         config::scene::nodeBudget);
  createBuffer(EBufferType::CPU_STORAGE,
               config::scene::getNodeTransformBufferSize(),
               scene.nodeTransformBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d unique skins.",
         config::scene::entityBudget);
  createBuffer(EBufferType::CPU_STORAGE,
               config::scene::getSkinTransformBufferSize(),
               scene.skinTransformBuffer, nullptr);

  RE_LOG(Log, "Allocating scene instance buffer for %d instances.", config::scene::nodeBudget);

  // TODO: Deprecate old instance buffers in favor of compute culling ones
  scene.instanceBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  //

  scene.culledInstanceDataBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  scene.culledDrawIndirectBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  scene.culledDrawCountBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  for (int8_t instanceBufferId = 0; instanceBufferId < MAX_FRAMES_IN_FLIGHT; ++instanceBufferId) {
    // TODO: Deprecate old instance buffers in favor of compute culling ones
    createBuffer(EBufferType::CPU_VERTEX, sizeof(RInstanceData) * config::scene::nodeBudget,
      scene.instanceBuffers[instanceBufferId], nullptr);
    //

    createBuffer(EBufferType::DGPU_VERTEX, sizeof(RInstanceData) * config::scene::nodeBudget,
      scene.culledInstanceDataBuffers[instanceBufferId], nullptr);
    createBuffer(EBufferType::DGPU_INDIRECT, sizeof(VkDrawIndexedIndirectCommand) * config::scene::nodeBudget,
      scene.culledDrawIndirectBuffers[instanceBufferId], nullptr);
    createBuffer(EBufferType::CPU_STORAGE, sizeof(RDrawIndirectInfo),
      scene.culledDrawCountBuffers[instanceBufferId], nullptr);
  }

  createBuffer(EBufferType::DGPU_STORAGE, sizeof(WInstanceDataEntry) * config::scene::nodeBudget,
    scene.instanceDataBuffer, nullptr);

  RE_LOG(Log, "Creating material storage buffer.");
  createBuffer(EBufferType::CPU_STORAGE, sizeof(RSceneFragmentPCB) * (config::scene::sampledImageBudget / RE_MAXTEXTURES),
    material.buffer, nullptr);

  RE_LOG(Log, "Creating order independent transparency buffers.");
  createBuffer(EBufferType::DGPU_STORAGE,
    config::renderWidth * config::renderHeight * sizeof(RTransparencyLinkedListNode) * RE_MAXTRANSPARENTLAYERS,
    scene.transparencyLinkedListBuffer, nullptr);

  createBuffer(EBufferType::DGPU_STORAGE, sizeof(RTransparencyLinkedListData),
    scene.transparencyLinkedListDataBuffer, &scene.transparencyLinkedListData);

  RE_LOG(Log, "Creating an exposure/luminance buffer.");  // 16 * 16 * float32 color = 1024 bytes
  createBuffer(EBufferType::CPU_STORAGE, 1024, postprocess.exposureStorageBuffer, nullptr);

  RE_LOG(Log, "Creating a general storage buffer.");
  createBuffer(EBufferType::DGPU_STORAGE, 32768, scene.generalBuffer, nullptr);
  copyDataToBuffer(system.occlusionOffsets.data(), sizeof(glm::vec4) * RE_OCCLUSIONSAMPLES, &scene.generalBuffer, 0u);

  command.indirectCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  for (uint8_t indirectBufferId = 0; indirectBufferId < MAX_FRAMES_IN_FLIGHT; ++indirectBufferId) {
    createBuffer(EBufferType::CPU_INDIRECT, sizeof(VkDrawIndexedIndirectCommand) * config::scene::nodeBudget * 10,
      command.indirectCommandBuffers[indirectBufferId], nullptr);
  }

  return RE_OK;
}

void core::MRenderer::destroySceneBuffers() {
  RE_LOG(Log, "Destroying scene buffers.");

  vmaDestroyBuffer(memAlloc, scene.vertexBuffer.buffer,
                   scene.vertexBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.indexBuffer.buffer,
                   scene.indexBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.modelTransformBuffer.buffer,
                   scene.modelTransformBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.nodeTransformBuffer.buffer,
                   scene.nodeTransformBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.skinTransformBuffer.buffer,
                   scene.skinTransformBuffer.allocation);

  for (int8_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex) {
    vmaDestroyBuffer(memAlloc, scene.instanceBuffers[frameIndex].buffer,
                     scene.instanceBuffers[frameIndex].allocation);
    vmaDestroyBuffer(memAlloc, command.indirectCommandBuffers[frameIndex].buffer,
                     command.indirectCommandBuffers[frameIndex].allocation);

    vmaDestroyBuffer(memAlloc, scene.culledInstanceDataBuffers[frameIndex].buffer,
      scene.culledInstanceDataBuffers[frameIndex].allocation);
    vmaDestroyBuffer(memAlloc, scene.culledDrawIndirectBuffers[frameIndex].buffer,
      scene.culledDrawIndirectBuffers[frameIndex].allocation);
    vmaDestroyBuffer(memAlloc, scene.culledDrawCountBuffers[frameIndex].buffer,
      scene.culledDrawCountBuffers[frameIndex].allocation);

    vmaDestroyBuffer(memAlloc, scene.sceneBuffers[frameIndex].buffer, scene.sceneBuffers[frameIndex].allocation);
    vmaDestroyBuffer(memAlloc, lighting.buffers[frameIndex].buffer, lighting.buffers[frameIndex].allocation);
  }

  vmaDestroyBuffer(memAlloc, postprocess.exposureStorageBuffer.buffer, postprocess.exposureStorageBuffer.allocation);

  vmaDestroyBuffer(memAlloc, scene.instanceDataBuffer.buffer, scene.instanceDataBuffer.allocation);
  
  vmaDestroyBuffer(memAlloc, material.buffer.buffer, material.buffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.transparencyLinkedListBuffer.buffer, scene.transparencyLinkedListBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.transparencyLinkedListDataBuffer.buffer, scene.transparencyLinkedListDataBuffer.allocation);

  vmaDestroyBuffer(memAlloc, scene.generalBuffer.buffer, scene.generalBuffer.allocation);
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

TResult core::MRenderer::setDefaultComputeJobs() {
  // BRDF LUT
  {
    RComputeJobInfo& info = compute.environmentJobInfo.LUT;
    info.pipeline = EComputePipeline::LUT;
    info.width = core::vulkan::LUTExtent / 8;
    info.height = core::vulkan::LUTExtent / 4;
    info.transtionToShaderReadOnly = true;
    info.pImageAttachments = { core::resources.getTexture(RTGT_BRDFMAP) };
  }

  // Environment irradiance map
  {
    RComputeJobInfo& info = compute.environmentJobInfo.irradiance;
    info.pipeline = EComputePipeline::EnvIrradiance;
    info.width = core::vulkan::envFilterExtent / 8;
    info.height = core::vulkan::envFilterExtent / 4;
    info.depth = 1;
    info.transtionToShaderReadOnly = true;
    info.useExtraImageViews = true;
    info.useExtraSamplerViews = false;
    info.pImageAttachments = { core::resources.getTexture(RTGT_ENVIRRAD) };
    info.pSamplerAttachments = { core::resources.getTexture(RTGT_ENV) };
    info.pushBlock.intValues.x = info.pImageAttachments[0]->texture.levelCount;
  }

  // Environment prefiltered map
  {
    RComputeJobInfo& info = compute.environmentJobInfo.prefiltered;
    info.pipeline = EComputePipeline::EnvFilter;
    info.width = core::vulkan::envFilterExtent / 8;
    info.height = core::vulkan::envFilterExtent / 4;
    info.depth = 1;
    info.transtionToShaderReadOnly = true;
    info.useExtraImageViews = true;
    info.useExtraSamplerViews = false;
    info.pImageAttachments = { core::resources.getTexture(RTGT_ENVFILTER) };
    info.pSamplerAttachments = { core::resources.getTexture(RTGT_ENV) };
    info.pushBlock.intValues.x = info.pImageAttachments[0]->texture.levelCount;
  }

  // Generate mipmaps (requires additional input data before submitting)
  {
    RComputeJobInfo& info = compute.imageJobInfo.mipmapping;
    info.pipeline = EComputePipeline::MipMap;
    info.depth = 1;
    info.useExtraImageViews = true;
    info.useExtraSamplerViews = false;
  }

  // Occlusion culling (requires modification depending on the total instance count and the current frame in flight index)
  {
    RComputeJobInfo& info = compute.sceneJobInfo.culling;
    info.pipeline = EComputePipeline::Culling;
    info.width = 1;
    info.height = 1;
    info.depth = 1;
    info.pBufferAttachments = { &scene.sceneBuffers[0],                 // 0
                                &scene.modelTransformBuffer,            // 1
                                &scene.nodeTransformBuffer,             // 2
                                &scene.instanceDataBuffer,              // 3
                                &scene.culledDrawIndirectBuffers[0],    // 4
                                &scene.culledInstanceDataBuffers[0],    // 5
                                &scene.culledDrawCountBuffers[0] };     // 6
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
  TResult chkResult = createDefaultSamplers();

  if (chkResult != RE_OK) {
    return chkResult;
  }

  chkResult = createImageTargets();

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

  setCamera(pCamera, true);

  // Set default lighting UBO data
  lighting.data.prefilteredCubeMipLevels = (float)math::getMipLevels(core::vulkan::envFilterExtent);

  // Set default post processing info
  postprocess.pBloomTexture = core::resources.getTexture(RTGT_PPBLOOM);
  postprocess.pExposureTexture = core::resources.getTexture(RTGT_EXPOSUREMAP);
  postprocess.pTAATexture = core::resources.getTexture(RTGT_PPTAA);
  postprocess.pPreviousFrameTexture = core::resources.getTexture(RTGT_PREVFRAME);

  postprocess.bloomSubRange.aspectMask = postprocess.pBloomTexture->texture.aspectMask;
  postprocess.bloomSubRange.baseArrayLayer = 0u;
  postprocess.bloomSubRange.layerCount = 1u;
  postprocess.bloomSubRange.baseMipLevel = 0u;
  postprocess.bloomSubRange.levelCount = 1u;

  postprocess.previousFrameCopy.extent = { config::renderWidth, config::renderHeight, 1 };
  postprocess.previousFrameCopy.srcOffset = { 0, 0, 0 };
  postprocess.previousFrameCopy.srcSubresource.aspectMask = postprocess.pTAATexture->texture.aspectMask;
  postprocess.previousFrameCopy.srcSubresource.baseArrayLayer = 0u;
  postprocess.previousFrameCopy.srcSubresource.layerCount = 1u;
  postprocess.previousFrameCopy.srcSubresource.mipLevel = 0u;
  postprocess.previousFrameCopy.dstOffset = postprocess.previousFrameCopy.srcOffset;
  postprocess.previousFrameCopy.dstSubresource = postprocess.previousFrameCopy.srcSubresource;

  postprocess.viewports.resize(postprocess.pBloomTexture->texture.levelCount);
  postprocess.scissors.resize(postprocess.pBloomTexture->texture.levelCount);
  
  for (uint8_t PPIndex = 0; PPIndex < postprocess.pBloomTexture->texture.levelCount; ++PPIndex) {
    const uint32_t currentWidth = postprocess.pBloomTexture->texture.width / (1 << PPIndex);
    const uint32_t currentHeight = postprocess.pBloomTexture->texture.height / (1 << PPIndex);

    postprocess.viewports[PPIndex].x = 0.0f;
    postprocess.viewports[PPIndex].y = static_cast<float>(currentHeight);
    postprocess.viewports[PPIndex].width = static_cast<float>(currentWidth);
    postprocess.viewports[PPIndex].height = -postprocess.viewports[PPIndex].y;
    postprocess.viewports[PPIndex].minDepth = 0.0f;
    postprocess.viewports[PPIndex].maxDepth = 1.0f;

    postprocess.scissors[PPIndex].offset = {0, 0};
    postprocess.scissors[PPIndex].extent = {currentWidth, currentHeight};
  }

  // Calculate a Halton Sequence for TAA jittering
  system.haltonJitter.resize(core::vulkan::haltonSequenceCount);
  math::getHaltonJitter(system.haltonJitter, config::renderWidth, config::renderHeight);

  // Generate random positions for ambient occlusion sampling
  system.occlusionOffsets.resize(RE_OCCLUSIONSAMPLES);
  for (uint32_t i = 0; i < RE_OCCLUSIONSAMPLES; ++i) {
    system.occlusionOffsets[i] =
      glm::vec4(math::random(-1.0f, 1.0f), math::random(-1.0f, 1.0f), math::random(0.0f, 1.0f), 0.0f);

    system.occlusionOffsets[i] = glm::normalize(system.occlusionOffsets[i]);
    system.occlusionOffsets[i] *= math::random(0.0f, 1.0f);

    float scale = (float)i / (float)RE_OCCLUSIONSAMPLES;
    scale = std::lerp(0.1f, 1.0f, scale * scale);
    system.occlusionOffsets[i] *= scale;
  }

  system.randomOffsets.resize(16);
  for (uint32_t j = 0; j < 16; ++j) {
    system.randomOffsets[j] = glm::vec4(math::random(-1.0f, 1.0f), math::random(-1.0f, 1.0f), math::random(0.0f, 1.0f), 0.0f);
  }

  // Setup transparency linked list
  scene.transparencySubRange.aspectMask = scene.pTransparencyStorageTexture->texture.aspectMask;
  scene.transparencySubRange.baseArrayLayer = 0u;
  scene.transparencySubRange.layerCount = 1u;
  scene.transparencySubRange.baseMipLevel = 0u;
  scene.transparencySubRange.levelCount = 1u;
  scene.transparencyLinkedListClearColor.uint32[0] = 0xFFFFFFFF;

  scene.transparencyLinkedListData.nodeCount = 0u;
  scene.transparencyLinkedListData.maxNodeCount = config::renderWidth * config::renderHeight * RE_MAXTRANSPARENTLAYERS;

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

  RE_LOG(Log, "Creating compute command buffers for %d frames.",
    MAX_FRAMES_IN_FLIGHT);

  command.buffersCompute.resize(MAX_FRAMES_IN_FLIGHT);

  for (uint8_t j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j) {
    command.buffersCompute[j] = createCommandBuffer(
      ECmdType::Compute, VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

    if (command.buffersCompute[j] == nullptr) {
      RE_LOG(Critical, "Failed to allocate compute command buffers.");
      return RE_CRITICAL;
    }
  }

  RE_LOG(Log, "Creating %d transfer command buffers.", MAX_TRANSFER_BUFFERS);

  command.buffersTransfer.resize(MAX_TRANSFER_BUFFERS);

  for (uint8_t k = 0; k < MAX_TRANSFER_BUFFERS; ++k) {
    command.buffersTransfer[k] = createCommandBuffer(
        ECmdType::Transfer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

    if (command.buffersTransfer[k] == nullptr) {
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
                       command.buffersCompute.data());

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
      RE_LOG(Critical, "Failed to create 'image available' semaphore.");

      return RE_CRITICAL;
    }

    if (vkCreateSemaphore(logicalDevice.device, &semInfo, nullptr,
                          &sync.semRenderFinished[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create 'render finished' semaphore.");

      return RE_CRITICAL;
    }

    if (vkCreateFence(logicalDevice.device, &fenInfo, nullptr,
                      &sync.fenceInFlight[i])) {
      RE_LOG(Critical, "failed to create 'in flight' fence.");

      return RE_CRITICAL;
    }
  }

  // Create asynchronously running threads
  RE_LOG(Log, "Creating entity update thread.");
  sync.asyncUpdateEntities.bindFunction(this, &MRenderer::updateBoundEntities);
  sync.asyncUpdateEntities.start();

  sync.asyncUpdateInstanceBuffers.bindFunction(this, &MRenderer::updateInstanceBuffer, &sync.cvInstanceDataReady);
  sync.asyncUpdateInstanceBuffers.start();

  return RE_OK;
}

void core::MRenderer::destroySyncObjects() {
  RE_LOG(Log, "Destroying synchronization objects.");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(logicalDevice.device, sync.semImgAvailable[i], nullptr);
    vkDestroySemaphore(logicalDevice.device, sync.semRenderFinished[i], nullptr);

    vkDestroyFence(logicalDevice.device, sync.fenceInFlight[i], nullptr);
  }

  RE_LOG(Log, "Stopping entity update thread.");
  sync.asyncUpdateEntities.stop();

  RE_LOG(Log, "Stopping instance buffers update thread.");
  sync.asyncUpdateInstanceBuffers.stop();
}

void core::MRenderer::updateSceneUBO(uint32_t currentImage) {
  scene.sceneBufferObject.view = view.pActiveCamera->getView();
  scene.sceneBufferObject.projection = view.pActiveCamera->getProjection();
  scene.sceneBufferObject.cameraPosition = view.pActiveCamera->getLocation();
  scene.sceneBufferObject.haltonJitter = system.haltonJitter[renderView.framesRendered % core::vulkan::haltonSequenceCount];
  scene.sceneBufferObject.clipData = view.pActiveCamera->getNearAndFarPlane();

  uint8_t* pSceneUBO = static_cast<uint8_t*>(scene.sceneBuffers[currentImage].allocInfo.pMappedData) +
                       config::scene::cameraBlockSize * view.pActiveCamera->getViewBufferIndex();

  memcpy(pSceneUBO, &scene.sceneBufferObject, sizeof(RSceneUBO));
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
  if (chkResult <= RE_ERRORLIMIT) chkResult = setRendererDefaults();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createSceneBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSetLayouts();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createPipelineLayouts();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createViewports();

  if (chkResult <= RE_ERRORLIMIT) chkResult = createDynamicRenderingPasses();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createComputePipelines();

  if (chkResult <= RE_ERRORLIMIT) chkResult = createSyncObjects();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorPool();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSets();

  if (chkResult <= RE_ERRORLIMIT) chkResult = createQueryPool();

  if (chkResult <= RE_ERRORLIMIT) chkResult = setDefaultComputeJobs();

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
  destroySamplers();
  destroyDescriptorPool();
  destroyQueryPool();
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
  return &scene.sceneBufferObject;
}

core::MRenderer::REnvironmentData* core::MRenderer::getEnvironmentData() {
  return &environment;
}

bool core::MRenderer::isLayoutTransitionEnabled() {
  return system.enableLayoutTransitions;
}

void core::MRenderer::updateLightingUBO(const int32_t frameIndex) {
  core::actors.updateLightingUBO(&lighting.data);

  lighting.data.aoMode = config::ambientOcclusionMode;

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