#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/managers/mgraphics.h"
#include "core/managers/mdebug.h"
#include "core/managers/mwindow.h"
#include "core/managers/mmodel.h"
#include "core/renderer/renderer.h"
#include "core/world/actors/acamera.h"

core::MGraphics::MGraphics() { RE_LOG(Log, "Creating graphics manager."); };

TResult core::MGraphics::createInstance() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = config::appTitle;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = config::engineTitle;
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = core::renderer::APIversion;

  return createInstance(&appInfo);
}

TResult core::MGraphics::createInstance(VkApplicationInfo* appInfo) {
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

TResult core::MGraphics::destroyInstance() {
  RE_LOG(Log, "Destroying Vulkan instance.");
  vkDestroyInstance(APIInstance, nullptr);
  return RE_OK;
}

TResult core::MGraphics::initialize() {
  TResult chkResult = RE_OK;

  chkResult = core::graphics->createInstance();

  // debug manager setup
  if (chkResult <= RE_ERRORLIMIT)
    chkResult = MDebug::get().create(core::graphics->APIInstance);

  if (chkResult <= RE_ERRORLIMIT) chkResult = createSurface();

  if (chkResult <= RE_ERRORLIMIT) chkResult = enumPhysicalDevices();
  if (chkResult <= RE_ERRORLIMIT) chkResult = initPhysicalDevice();
  if (chkResult <= RE_ERRORLIMIT) chkResult = initLogicalDevice();

  if (chkResult <= RE_ERRORLIMIT) chkResult = createMemAlloc();

  if (chkResult <= RE_ERRORLIMIT)
    chkResult =
        initSwapChain(core::renderer::format, core::renderer::colorSpace,
                      core::renderer::presentMode);
  updateAspectRatio();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createRenderPass();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createGraphicsPipeline();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createFramebuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCommandPools();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCommandBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createSyncObjects();

  MModel::get().createMesh();
  bindMesh(MModel::get().meshes.back().get());
  if (chkResult <= RE_ERRORLIMIT) chkResult = createMVPBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorPool();
  //if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSetLayouts();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createDescriptorSets();

  return chkResult;
}

void core::MGraphics::deinitialize() {
  waitForSystemIdle();

  destroySwapChain();
  destroySyncObjects();
  destroyCommandBuffers();
  destroyCommandPools();
  destroyGraphicsPipeline();
  destroyRenderPass();
  destroySurface();
  MModel::get().destroyAllMeshes();
  destroyDescriptorPool();
  destroyMVPBuffers();
  destroyMemAlloc();
  if(bRequireValidationLayers) MDebug::get().destroy(APIInstance);
  destroyLogicalDevice();
  destroyInstance();
}

TResult core::MGraphics::createMemAlloc() {
  RE_LOG(Log, "initializing Vulkan memory allocator.");

  VmaAllocatorCreateInfo allocCreateInfo{};
  allocCreateInfo.instance = APIInstance;
  allocCreateInfo.physicalDevice = physicalDevice.device;
  allocCreateInfo.device = logicalDevice.device;
  allocCreateInfo.vulkanApiVersion = core::renderer::APIversion;

  if (vmaCreateAllocator(&allocCreateInfo, &memAlloc) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create Vulkan memory allocator.");
    return RE_CRITICAL;
  };

  return RE_OK;
}

void core::MGraphics::destroyMemAlloc() {
  RE_LOG(Log, "Destroying Vulkan memory allocator.");
  vmaDestroyAllocator(memAlloc);
}

void core::MGraphics::waitForSystemIdle() {
  vkQueueWaitIdle(logicalDevice.queues.graphics);
  vkQueueWaitIdle(logicalDevice.queues.present);
  vkDeviceWaitIdle(logicalDevice.device);
}

TResult core::MGraphics::createSurface() {
  RE_LOG(Log, "Creating rendering surface.");

  if (glfwCreateWindowSurface(APIInstance, MWindow::get().window(), nullptr,
                              &surface) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create rendering surface.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MGraphics::destroySurface() {
  RE_LOG(Log, "Destroying drawing surface.");
  vkDestroySurfaceKHR(APIInstance, surface, nullptr);
}

uint32_t core::MGraphics::bindMesh(WMesh* pMesh) {
  if (!pMesh) {
    RE_LOG(Error, "no mesh provided.");
    return -1;
  }

  system.meshes.emplace_back(pMesh);
  return (uint32_t)system.meshes.size() - 1;
}

TResult core::MGraphics::createDescriptorSetLayouts() {
  VkDescriptorSetLayoutBinding uboMVPBind{};
  uboMVPBind.binding = 0;                                         // binding location in a shader
  uboMVPBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // type of binding
  uboMVPBind.descriptorCount = 1;                                 // binding can be an array of UBOs, but MVP is a single UBO
  uboMVPBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;             // will be bound to vertex shader
  uboMVPBind.pImmutableSamplers = nullptr;                        // for texture samplers

  VkDescriptorSetLayoutCreateInfo uboMVPInfo{};
  uboMVPInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  uboMVPInfo.bindingCount = 1;
  uboMVPInfo.pBindings = &uboMVPBind;

  for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    system.descSetLayouts.emplace_back();
    if (vkCreateDescriptorSetLayout(logicalDevice.device, &uboMVPInfo, nullptr,
                                    &system.descSetLayouts.back()) !=
        VK_SUCCESS) {
      system.descSetLayouts.erase(system.descSetLayouts.end());
      RE_LOG(Critical, "Failed to create MVP matrix descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  return RE_OK;
}

void core::MGraphics::destroyDescriptorSetLayouts(){
  RE_LOG(Log, "Removing descriptor set layouts.");

  for (auto& it : system.descSetLayouts) {
    vkDestroyDescriptorSetLayout(logicalDevice.device, it, nullptr);
  }
}

TResult core::MGraphics::createDescriptorPool() {
  RE_LOG(Log, "Creating descriptor pool.");

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(logicalDevice.device, &poolInfo, nullptr,
                             &system.descPool) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create descriptor pool.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MGraphics::destroyDescriptorPool() {
  RE_LOG(Log, "Destroying descriptor pool.");
  vkDestroyDescriptorPool(logicalDevice.device, system.descPool, nullptr);
}

TResult core::MGraphics::createDescriptorSets() {
  VkDescriptorSetAllocateInfo setAllocInfo{};
  setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool = system.descPool;
  setAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  setAllocInfo.pSetLayouts = system.descSetLayouts.data();

  system.descSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(logicalDevice.device, &setAllocInfo,
                               system.descSets.data()) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to allocate descriptor sets.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MGraphics::destroyDescriptorSets() {}

TResult core::MGraphics::createMVPBuffers() {
  // each frame will require a separate buffer, so 2 FIF would need buffers * 2
  view.buffersMVP.resize(MAX_FRAMES_IN_FLIGHT);

  VkDeviceSize uboMVPsize = sizeof(RMVPMatrices);

  for (int i = 0; i < view.buffersMVP.size();
       i += MAX_FRAMES_IN_FLIGHT) {
    createBuffer(EBCMode::CPU_UNIFORM, uboMVPsize,
                 view.buffersMVP[i].buffer,
                 view.buffersMVP[i].allocation, getMVP(),
                 &view.buffersMVP[i].allocInfo);
    createBuffer(EBCMode::CPU_UNIFORM, uboMVPsize,
                 view.buffersMVP[i + 1].buffer,
                 view.buffersMVP[i + 1].allocation, getMVP(),
                 &view.buffersMVP[i + 1].allocInfo);
  }

  return RE_OK;
}

void core::MGraphics::destroyMVPBuffers() {
  for (auto& it : view.buffersMVP) {
    vmaDestroyBuffer(memAlloc, it.buffer, it.allocation);
  }
}

void core::MGraphics::updateMVPBuffer(uint32_t currentImage) {
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   currentTime - startTime)
                   .count();

  auto r = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                       glm::vec3(0.0f, 0.0f, 1.0f));
  memcpy(view.buffersMVP[currentImage].allocInfo.pMappedData,
         updateMVP(&r), sizeof(RMVPMatrices));
}

RMVPMatrices* core::MGraphics::getMVP() {
  return &view.modelViewProjection;
}

RMVPMatrices* core::MGraphics::updateMVP(glm::mat4* pTransform) {
  view.modelViewProjection = {glm::mat4(1.0f), view.pActiveCamera->view(),
          view.pActiveCamera->projection()};

  return &view.modelViewProjection;
}

VkShaderModule core::MGraphics::createShaderModule(std::vector<char>& shaderCode) {
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

TResult core::MGraphics::checkInstanceValidationLayers() {
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

std::vector<const char*> core::MGraphics::getRequiredInstanceExtensions() {
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

std::vector<VkExtensionProperties> core::MGraphics::getInstanceExtensions() {
  uint32_t extensionCount = 0;
  std::vector<VkExtensionProperties> extensionProperties;

  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  extensionProperties.resize(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensionProperties.data());

  return extensionProperties;
}