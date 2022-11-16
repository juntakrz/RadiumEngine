#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/managers/mgraphics.h"
#include "core/managers/mdebug.h"
#include "core/managers/mwindow.h"
#include "core/managers/mmodel.h"
#include "core/renderer/renderer.h"
#include "core/world/actors/acamera.h"

MGraphics::MGraphics() { RE_LOG(Log, "Creating graphics manager."); };

TResult MGraphics::createInstance() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = config::appTitle;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = config::engineTitle;
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = core::renderer::APIversion;

  return createInstance(&appInfo);
}

TResult MGraphics::createInstance(VkApplicationInfo* appInfo) {
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

TResult MGraphics::destroyInstance() {
  RE_LOG(Log, "Destroying Vulkan instance.");
  vkDestroyInstance(APIInstance, nullptr);
  return RE_OK;
}

TResult MGraphics::initialize() {
  TResult chkResult = RE_OK;

  chkResult = mgrGfx->createInstance();

  // debug manager setup
  if (chkResult <= RE_ERRORLIMIT)
    chkResult = mgrDbg->create(mgrGfx->APIInstance);

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

  mgrModel->createMesh();
  bindMesh(mgrModel->meshes.back().get());
  createUniformBuffers();

  return chkResult;
}

void MGraphics::deinitialize() {
  waitForSystemIdle();

  destroySwapChain();
  destroySyncObjects();
  destroyCommandBuffers();
  destroyCommandPools();
  destroyGraphicsPipeline();
  destroyRenderPass();
  destroySurface();
  mgrModel->destroyAllMeshes();
  destroyUniformBuffers();
  destroyMemAlloc();
  if(bRequireValidationLayers) mgrDbg->destroy(APIInstance);
  destroyLogicalDevice();
  destroyInstance();
}

TResult MGraphics::createMemAlloc() {
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

void MGraphics::destroyMemAlloc() {
  RE_LOG(Log, "Destroying Vulkan memory allocator.");
  vmaDestroyAllocator(memAlloc);
}

void MGraphics::waitForSystemIdle() {
  vkQueueWaitIdle(logicalDevice.queues.graphics);
  vkQueueWaitIdle(logicalDevice.queues.present);
  vkDeviceWaitIdle(logicalDevice.device);
}

TResult MGraphics::createSurface() {
  RE_LOG(Log, "Creating rendering surface.");

  if (glfwCreateWindowSurface(APIInstance, MWindow::get().window(), nullptr,
                              &surface) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create rendering surface.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

void MGraphics::destroySurface() {
  RE_LOG(Log, "Destroying drawing surface.");
  vkDestroySurfaceKHR(APIInstance, surface, nullptr);
}

uint32_t MGraphics::bindMesh(WMesh* pMesh) {
  if (!pMesh) {
    RE_LOG(Error, "no mesh provided.");
    return -1;
  }

  sSystem.meshes.emplace_back(pMesh);
  return (uint32_t)sSystem.meshes.size() - 1;
}

TResult MGraphics::createDescriptorSetLayouts() {
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

  sSystem.descSetLayouts.emplace_back();
  if (vkCreateDescriptorSetLayout(logicalDevice.device, &uboMVPInfo, nullptr,
                                  &sSystem.descSetLayouts.back()) !=
      VK_SUCCESS) {
    sSystem.descSetLayouts.erase(sSystem.descSetLayouts.end());
    RE_LOG(Critical, "Failed to create MVP matrix descriptor set layout.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void MGraphics::destroyDescriptorSetLayouts(){
  for (auto& it : sSystem.descSetLayouts) {
    vkDestroyDescriptorSetLayout(logicalDevice.device, it, nullptr);
  }
}

TResult MGraphics::createUniformBuffers() {
  // each frame will require a separate buffer, so 2 FIF would need buffers * 2
  sRender.buffersUniform.resize(MAX_FRAMES_IN_FLIGHT);

  VkDeviceSize uboMVPsize = sizeof(RMVPMatrices);

  for (int i = 0; i < sRender.buffersUniform.size();
       i += MAX_FRAMES_IN_FLIGHT) {
    createBuffer(EBCMode::CPU_UNIFORM, uboMVPsize,
                 sRender.buffersUniform[i].buffer,
                 sRender.buffersUniform[i].allocation, getMVP(),
                 &sRender.buffersUniform[i].allocInfo);
    createBuffer(EBCMode::CPU_UNIFORM, uboMVPsize,
                 sRender.buffersUniform[i + 1].buffer,
                 sRender.buffersUniform[i + 1].allocation, getMVP(),
                 &sRender.buffersUniform[i + 1].allocInfo);
  }

  return RE_OK;
}

void MGraphics::destroyUniformBuffers() {
  for (auto& it : sRender.buffersUniform) {
    vmaDestroyBuffer(memAlloc, it.buffer, it.allocation);
  }
}

void MGraphics::updateUniformBuffers() {
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   currentTime - startTime)
                   .count();
}

RMVPMatrices* MGraphics::getMVP() {
  return &sRender.modelViewProjection;
}

RMVPMatrices* MGraphics::getMVP(ABase* pActor) {
  sRender.modelViewProjection = {glm::mat4(1.0f), sRender.pActiveCamera->view(),
          sRender.pActiveCamera->projection()};

  return &sRender.modelViewProjection;
}

VkShaderModule MGraphics::createShaderModule(std::vector<char>& shaderCode) {
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

TResult MGraphics::checkInstanceValidationLayers() {
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

std::vector<const char*> MGraphics::getRequiredInstanceExtensions() {
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

std::vector<VkExtensionProperties> MGraphics::getInstanceExtensions() {
  uint32_t extensionCount = 0;
  std::vector<VkExtensionProperties> extensionProperties;

  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  extensionProperties.resize(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensionProperties.data());

  return extensionProperties;
}