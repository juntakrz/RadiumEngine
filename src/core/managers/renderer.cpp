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
  RE_LOG(Log, "Allocating scene buffer for %d vertices.",
         config::scene::vertexBudget);
  createBuffer(EBufferMode::DGPU_VERTEX, config::scene::getVertexBufferSize(),
               scene.vertexBuffer, nullptr);

  RE_LOG(Log, "Allocating scene buffer for %d indices.",
         config::scene::indexBudget);
  createBuffer(EBufferMode::DGPU_INDEX, config::scene::getIndexBufferSize(),
               scene.indexBuffer, nullptr);

  return RE_OK;
}

void core::MRenderer::destroySceneBuffers() {
  RE_LOG(Log, "Destroying scene buffers.");
  vmaDestroyBuffer(memAlloc, scene.vertexBuffer.buffer,
                   scene.vertexBuffer.allocation);
  vmaDestroyBuffer(memAlloc, scene.indexBuffer.buffer,
                   scene.indexBuffer.allocation);
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
  // using map data e.g. max preloaded textures plus 256 for headroom
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = (1536 + 256) * MAX_FRAMES_IN_FLIGHT;

  // model nodes
  // TODO: rewrite so that descriptorCounts are calculated by objects/textures
  // using map data e.g. max preloaded model nodes plus 256 for headroom
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[2].descriptorCount = (1024 + 256) * MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets =
      static_cast<uint32_t>(poolSizes.size()) *
      (MAX_FRAMES_IN_FLIGHT)*10;  // need to calculate better number
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
  // each frame will require a separate buffer, so 2 frames in flight would
  // require buffers * 2
  view.modelViewProjectionBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  lighting.buffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkDeviceSize uboMVPSize = sizeof(RSceneUBO);
  VkDeviceSize uboLightingSize = sizeof(RLightingUBO);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    createBuffer(EBufferMode::CPU_UNIFORM, uboMVPSize,
                 view.modelViewProjectionBuffers[i], getSceneUBO());
    createBuffer(EBufferMode::CPU_UNIFORM, uboLightingSize, lighting.buffers[i],
                 &lighting.data);
  }

  // create environment buffer
  VkDeviceSize minBufferAlignment =
      physicalDevice.properties.limits.minUniformBufferOffsetAlignment;
  VkDeviceSize alignedSize = (sizeof(REnvironmentUBO) + minBufferAlignment - 1) &
                         ~(minBufferAlignment - 1);
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

  // front target texture used as a source for environment cubemap sides
  std::string rtName = RT_FRONT;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.asCubemap = false;
  textureInfo.width = core::vulkan::envFilterExtent;
  textureInfo.height = textureInfo.width;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

  // default cubemap texture as a copy target
  rtName = RT_ENVMAP;
  uint32_t dimension = core::vulkan::envFilterExtent;
  uint32_t mipLevels = static_cast<uint32_t>(floor(log2(dimension))) + 1;

  textureInfo = RTextureInfo{};
  textureInfo.name = rtName;
  textureInfo.asCubemap = true;
  textureInfo.width = dimension;
  textureInfo.height = textureInfo.width;
  textureInfo.format = core::vulkan::formatHDR16;
  textureInfo.mipLevels = mipLevels;
  textureInfo.targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textureInfo.usageFlags =
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  pNewTexture = core::resources.createTexture(&textureInfo);

  if (!pNewTexture) {
    RE_LOG(Critical, "Failed to create texture \"%s\".", rtName.c_str());
    return RE_CRITICAL;
  }

  rtName = RT_IRRADMAP;
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

  return createDepthTarget();
}

TResult core::MRenderer::createDepthTarget() {
#ifndef NDEBUG
  RE_LOG(Log, "Creating depth/stencil target.");
#endif

  // may not be supported by every GPU, maybe write a format checker?
  if (TResult result = setDepthStencilFormat() != RE_OK) {
    return result;
  };

  // default depth/stencil texture
  std::string rtName = RT_DEPTH;

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

  return RE_OK;
}

VkPipelineLayout& core::MRenderer::getPipelineLayout(EPipelineLayout type) {
  return system.layouts.at(type);
}

VkPipeline& core::MRenderer::getPipeline(EPipeline type) {
  // not error checked
  return system.pipelines.at(type);
}

bool core::MRenderer::checkPipeline(uint32_t pipelineFlags,
                                    EPipeline pipelineFlag) {
  return pipelineFlags & pipelineFlag;
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

  memcpy(view.modelViewProjectionBuffers[currentImage].allocInfo.pMappedData,
         &view.worldViewProjectionData, sizeof(RSceneUBO));
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
  if (chkResult <= RE_ERRORLIMIT) chkResult = createImageTargets();           // render targets, e.g. front, cube, depth
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSetLayouts();

  if (chkResult <= RE_ERRORLIMIT) chkResult = createRenderPasses();           // A
  if (chkResult <= RE_ERRORLIMIT) chkResult = createGraphicsPipelines();      // B
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDefaultFramebuffers();           // C
  if (chkResult <= RE_ERRORLIMIT) chkResult = configureRenderPasses();        // connect A, B, C together

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