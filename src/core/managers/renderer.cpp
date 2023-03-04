#include "pch.h"
#include "vk_mem_alloc.h"
#include "util/math.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/debug.h"
#include "core/managers/window.h"
#include "core/managers/actors.h"
#include "core/managers/materials.h"
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
  if (chkResult <= RE_ERRORLIMIT) chkResult = createRenderPass();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSetLayouts();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createGraphicsPipeline();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createFramebuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCommandPools();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCommandBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createSyncObjects();

  // delete this code after model loading/creation code is finished
  core::actors.createPawn("plane0");
  core::world.createModel(EWPrimitive::Sphere, "mdlPlane", 16, 0);
  WModel* pModel = core::world.getModel("mdlPlane");
  APawn* pPawn = core::actors.getPawn("plane0");
  pPawn->setModel(pModel);

  //bindPrimitive(pModel->getPrimitives(), pModel->getPrimitiveBindsIndex());

  core::world.loadModelFromFile("content/models/test/scene.gltf", "mdlTest");

  WModel* pTestModel = core::world.getModel("mdlTest");
  bindPrimitive(pTestModel->getPrimitives(), pModel->getPrimitiveBindsIndex());
  //

  if (chkResult <= RE_ERRORLIMIT) chkResult = createMVPBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorPool();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSets();

  return chkResult;
}

void core::MRenderer::deinitialize() {
  waitForSystemIdle();

  destroySwapChain();
  destroySyncObjects();
  destroyCommandBuffers();
  destroyCommandPools();
  destroyGraphicsPipeline();
  destroyRenderPass();
  destroySurface();
  core::actors.destroyAllPawns();
  core::world.destroyAllModels();
  core::materials.destroyAllTextures();
  destroyDescriptorPool();
  destroyMVPBuffers();
  destroyMemAlloc();
  if(bRequireValidationLayers) MDebug::get().destroy(APIInstance);
  destroyLogicalDevice();
  destroyInstance();
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

void core::MRenderer::waitForSystemIdle() {
  vkQueueWaitIdle(logicalDevice.queues.graphics);
  vkQueueWaitIdle(logicalDevice.queues.present);
  vkDeviceWaitIdle(logicalDevice.device);
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

TResult core::MRenderer::createDescriptorSetLayouts() {
  // layout for model view projection matrices for vertex shader
  VkDescriptorSetLayoutBinding uboMVPLayout{};
  uboMVPLayout.binding = 0;                                         // binding location in a shader
  uboMVPLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // type of binding
  uboMVPLayout.descriptorCount = 1;                                 // binding can be an array of UBOs, but MVP is a single UBO
  uboMVPLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;             // will be bound to vertex shader
  uboMVPLayout.pImmutableSamplers = nullptr;                        // for image samplers

  VkDescriptorSetLayoutCreateInfo uboMVPInfo{};
  uboMVPInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  uboMVPInfo.bindingCount = 1;
  uboMVPInfo.pBindings = &uboMVPLayout;

  if (vkCreateDescriptorSetLayout(logicalDevice.device, &uboMVPInfo, nullptr,
                                  &system.descriptorSetLayout) !=
      VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create MVP matrix descriptor set layout.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroyDescriptorSetLayouts(){
  RE_LOG(Log, "Removing descriptor set layouts.");

  vkDestroyDescriptorSetLayout(logicalDevice.device, system.descriptorSetLayout,
                               nullptr);
}

TResult core::MRenderer::createDescriptorPool() {
  RE_LOG(Log, "Creating descriptor pool.");

  // the number of descriptors in the given pool
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
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

  RE_LOG(Log, "Creating descriptor sets.");

  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             system.descriptorSetLayout);

  VkDescriptorSetAllocateInfo setAllocInfo{};
  setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool = system.descriptorPool;
  setAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  setAllocInfo.pSetLayouts = layouts.data();

  system.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

  if (vkAllocateDescriptorSets(logicalDevice.device, &setAllocInfo,
                               system.descriptorSets.data()) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to allocate descriptor sets.");
    return RE_CRITICAL;
  }

  RE_LOG(Log, "Populating descriptor sets.");

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    
    uint32_t descriptorCount = 1u;

    // data for descriptor set
    VkDescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo.buffer = view.modelViewProjectionBuffers[i].buffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(RSModelViewProjection);

    // settings used for writing to descriptor sets
    VkWriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = system.descriptorSets[i];
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.descriptorCount = descriptorCount;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
    writeDescriptorSet.pImageInfo = nullptr;
    writeDescriptorSet.pTexelBufferView = nullptr;
    writeDescriptorSet.pNext = nullptr;

    vkUpdateDescriptorSets(logicalDevice.device, descriptorCount,
                           &writeDescriptorSet, 0, nullptr);
  }

  return RE_OK;
}

TResult core::MRenderer::createMVPBuffers() {
  // each frame will require a separate buffer, so 2 frames in flight would require buffers * 2
  view.modelViewProjectionBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkDeviceSize uboMVPsize = sizeof(RSModelViewProjection);

  for (int i = 0; i < view.modelViewProjectionBuffers.size();
       i += MAX_FRAMES_IN_FLIGHT) {
    createBuffer(EBufferMode::CPU_UNIFORM, uboMVPsize, view.modelViewProjectionBuffers[i], getMVPview());
    createBuffer(EBufferMode::CPU_UNIFORM, uboMVPsize, view.modelViewProjectionBuffers[i + 1], getMVPview());
  }

  return RE_OK;
}

void core::MRenderer::destroyMVPBuffers() {
  for (auto& it : view.modelViewProjectionBuffers) {
    vmaDestroyBuffer(memAlloc, it.buffer, it.allocation);
  }
}

void core::MRenderer::updateModelViewProjectionBuffers(uint32_t currentImage) {
  float time = core::time.getTimeSinceInitialization();

  // rewrite this and UpdateMVP method to use data from the current/provided camera
  glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.2f));
  view.modelViewProjectionData.model =
      glm::rotate(t, glm::mod(time * glm::radians(90.0f), math::twoPI), glm::vec3(0.0f, 0.3f, 1.0f));

  view.modelViewProjectionData.view = view.pActiveCamera->getView();
  view.modelViewProjectionData.projection = view.pActiveCamera->getProjection();

  memcpy(view.modelViewProjectionBuffers[currentImage].allocInfo.pMappedData,
         &view.modelViewProjectionData, sizeof(RSModelViewProjection));
}

RSModelViewProjection* core::MRenderer::getMVPview() {
  return &view.modelViewProjectionData;
}

RSModelViewProjection* core::MRenderer::updateModelViewProjection(glm::mat4* pTransform) {
  view.modelViewProjectionData = {glm::mat4(1.0f), view.pActiveCamera->getView(),
          view.pActiveCamera->getProjection()};

  return &view.modelViewProjectionData;
}

VkShaderModule core::MRenderer::createShaderModule(
    std::vector<uint8_t>& shaderCode) {
  VkShaderModuleCreateInfo smInfo{};
  smInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  smInfo.codeSize = shaderCode.size();
  smInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());

  VkShaderModule shaderModule;
  if ((vkCreateShaderModule(logicalDevice.device, &smInfo, nullptr,
                            &shaderModule) != VK_SUCCESS)) {
    RE_LOG(Warning, "failed to create requested shader module.");
    return VK_NULL_HANDLE;
  };

  return shaderModule;
}

TResult core::MRenderer::checkInstanceValidationLayers() {
  uint32_t layerCount = 0;
  std::vector<VkLayerProperties> availableValidationLayers;
  VkResult checkResult;
  bool bRequestedLayersAvailable = false;
  std::string errorLayer;

  // enumerate instance layers
  checkResult = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  availableValidationLayers.resize(layerCount);
  checkResult = vkEnumerateInstanceLayerProperties(
    &layerCount, availableValidationLayers.data());
  if (checkResult != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to enumerate instance layer properties.");
    return RE_CRITICAL;
  }

  // check if all requested validation layers are available
  for (const char* requestedLayer : debug::validationLayers) {
    bRequestedLayersAvailable = false;
    for (const auto& availableLayer : availableValidationLayers) {
      if (strcmp(requestedLayer, availableLayer.layerName) == 0) {
        bRequestedLayersAvailable = true;
        break;
      }
    }
    if (!bRequestedLayersAvailable) {
      errorLayer = std::string(requestedLayer);
      break;
    }
  }

  if (!bRequestedLayersAvailable) {
    RE_LOG(Critical,
           "Failed to detect all requested validation layers. Layer '%s' not "
           "present.",
           errorLayer.c_str());
    return RE_CRITICAL;
  }

  return RE_OK;
}

std::vector<const char*> core::MRenderer::getRequiredInstanceExtensions() {
  uint32_t extensionCount = 0;
  const char** ppExtensions;

  ppExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
  std::vector<const char*> requiredExtensions(ppExtensions,
                                              ppExtensions + extensionCount);

  if (bRequireValidationLayers) {
    requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return requiredExtensions;
}

std::vector<VkExtensionProperties> core::MRenderer::getInstanceExtensions() {
  uint32_t extensionCount = 0;
  std::vector<VkExtensionProperties> extensionProperties;

  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  extensionProperties.resize(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensionProperties.data());

  return extensionProperties;
}