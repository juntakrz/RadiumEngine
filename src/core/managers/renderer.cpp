#include "pch.h"
#include "vk_mem_alloc.h"
#include "util/math.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/debug.h"
#include "core/managers/window.h"
#include "core/managers/actors.h"
#include "core/managers/resources.h"
#include "core/managers/time.h"
#include "core/managers/world.h"
#include "core/world/actors/camera.h"

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

TResult core::MRenderer::createDescriptorSetLayouts() {
  // layout: shader binding / descriptor type / count / shader stage / immutable
  // samplers

  // scene matrices and environmental maps
  // 0 - MVP matrix
  // 1 - lighting variables
  // 2 - TODO
  // 3 - TODO
  // 4 - TODO
  {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.scene) != VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create scene descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // standard set of 6 texture maps
  // 0 - baseColor
  // 1 - normal
  // 2 - metalness / roughness
  // 3 - ambient occlusion
  // 4 - emissive
  // 5 - extra
  {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.material) != VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create material descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // model mesh matrices
  {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,
         nullptr},
    };

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.mesh) != VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create model node descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  return RE_OK;
}

void core::MRenderer::destroyDescriptorSetLayouts() {
  RE_LOG(Log, "Removing descriptor set layouts.");

  vkDestroyDescriptorSetLayout(logicalDevice.device,
                               system.descriptorSetLayouts.scene, nullptr);
  vkDestroyDescriptorSetLayout(logicalDevice.device,
                               system.descriptorSetLayouts.material, nullptr);
  vkDestroyDescriptorSetLayout(logicalDevice.device,
                               system.descriptorSetLayouts.mesh, nullptr);
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

TResult core::MRenderer::createDescriptorSets() {
  RE_LOG(Log, "Creating renderer descriptor sets.");

  std::vector<VkDescriptorSetLayout> setLayouts(
      MAX_FRAMES_IN_FLIGHT, system.descriptorSetLayouts.scene);

  VkDescriptorSetAllocateInfo setAllocInfo{};
  setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool = system.descriptorPool;
  setAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  setAllocInfo.pSetLayouts = setLayouts.data();

  system.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

  if (vkAllocateDescriptorSets(logicalDevice.device, &setAllocInfo,
                               system.descriptorSets.data()) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to allocate descriptor sets.");
    return RE_CRITICAL;
  }

  RE_LOG(Log, "Populating descriptor sets.");

  uint32_t descriptorCount = 2u;

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    // model*view*projection data for descriptor set
    VkDescriptorBufferInfo descriptorBufferInfoMVP;
    descriptorBufferInfoMVP.buffer = view.modelViewProjectionBuffers[i].buffer;
    descriptorBufferInfoMVP.offset = 0;
    descriptorBufferInfoMVP.range = sizeof(RSceneUBO);

    // lighting data for descriptor set
    VkDescriptorBufferInfo descriptorBufferInfoLighting;
    descriptorBufferInfoLighting.buffer = lighting.buffers[i].buffer;
    descriptorBufferInfoLighting.offset = 0;
    descriptorBufferInfoLighting.range = sizeof(RLightingUBO);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets(descriptorCount);

    // settings used for writing to MVP descriptor set
    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].dstSet = system.descriptorSets[i];
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].dstArrayElement = 0;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].pBufferInfo = &descriptorBufferInfoMVP;
    writeDescriptorSets[0].pImageInfo = nullptr;
    writeDescriptorSets[0].pTexelBufferView = nullptr;
    writeDescriptorSets[0].pNext = nullptr;

    // settings used for writing to lighting descriptor set
    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].dstSet = system.descriptorSets[i];
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].pBufferInfo = &descriptorBufferInfoLighting;
    /*
    // environment image data
    writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[2].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[2].descriptorCount = 1;
    writeDescriptorSets[2].dstSet = system.descriptorSets[i];
    writeDescriptorSets[2].dstBinding = 2;
    writeDescriptorSets[2].pImageInfo = &textures.irradianceCube.descriptor;

    writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[3].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[3].descriptorCount = 1;
    writeDescriptorSets[3].dstSet = system.descriptorSets[i];
    writeDescriptorSets[3].dstBinding = 3;
    writeDescriptorSets[3].pImageInfo = &textures.prefilteredCube.descriptor;

    writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[4].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[4].descriptorCount = 1;
    writeDescriptorSets[4].dstSet = system.descriptorSets[i];
    writeDescriptorSets[4].dstBinding = 4;
    writeDescriptorSets[4].pImageInfo = &textures.lutBrdf.descriptor;
    */
    vkUpdateDescriptorSets(logicalDevice.device, descriptorCount,
                           writeDescriptorSets.data(), 0, nullptr);
  }

  return RE_OK;
}

TResult core::MRenderer::createDepthResources() {
  RE_LOG(Log, "Creating depth/stencil resources.");

  // may not be supported by every GPU, maybe write a format checker?
  if (TResult result = setDepthStencilFormat() != RE_OK) {
    return result;
  };

  VkImageCreateInfo imageCreateInfo{};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.extent = {swapchain.imageExtent.width,
                            swapchain.imageExtent.height, 1};
  imageCreateInfo.format = images.depth.format;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VmaAllocationCreateInfo depthAllocationInfo{};
  depthAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  depthAllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  if (vmaCreateImage(memAlloc, &imageCreateInfo, &depthAllocationInfo,
                     &images.depth.image, &images.depth.allocation,
                     &images.depth.allocInfo) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create depth/stencil image.");
    return RE_CRITICAL;
  }

  images.depth.view =
      createImageView(images.depth.image, images.depth.format, 1, 1);

  if (!images.depth.view) {
    RE_LOG(Critical, "Failed to create depth/stencil image view.");
    return RE_CRITICAL;
  }

  transitionImageLayout(
      images.depth.image, images.depth.format, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ECmdType::Graphics);

  return RE_OK;
}

void core::MRenderer::destroyDepthResources() {
  vkDestroyImageView(logicalDevice.device, images.depth.view, nullptr);
  vmaDestroyImage(memAlloc, images.depth.image, images.depth.allocation);
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
    initSwapChain(core::vulkan::format, core::vulkan::colorSpace,
                      core::vulkan::presentMode);
  updateAspectRatio();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCoreCommandPools();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCoreCommandBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createSceneBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDepthResources();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createRenderPass();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSetLayouts();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createGraphicsPipelines();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createFramebuffers();
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
  destroyRenderPass();
  destroyDepthResources();
  destroySceneBuffers();
  destroySurface();
  core::actors.destroyAllPawns();
  core::world.destroyAllModels();
  core::materials.destroyAllTextures();
  destroyDescriptorPool();
  destroyUniformBuffers();
  destroyMemAlloc();
  if(bRequireValidationLayers) MDebug::get().destroy(APIInstance);
  destroyLogicalDevice();
  destroyInstance();
}

const RDescriptorSetLayouts* core::MRenderer::getDescriptorSetLayouts() const {
  return &system.descriptorSetLayouts;
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