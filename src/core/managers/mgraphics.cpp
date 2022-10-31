#include "pch.h"
#include "core/managers/mgraphics.h"
#include "core/managers/mdebug.h"
#include "core/managers/mwindow.h"
#include "core/renderer/renderer.h"

MGraphics::MGraphics() { RE_LOG(Log, "Creating graphics manager."); };

TResult MGraphics::createInstance() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = settings.appTitle;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = settings.engineTitle;
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

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
  if (chkResult <= RE_ERRORLIMIT) chkResult = initDefaultPhysicalDevice();
  if (chkResult <= RE_ERRORLIMIT)
    chkResult =
        initSwapChain(core::renderer::format, core::renderer::colorSpace,
                      core::renderer::presentMode);
  if (chkResult <= RE_ERRORLIMIT) chkResult = createRenderPass();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createGraphicsPipeline();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createFramebuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCommandPool();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createCommandBuffers();
  if (chkResult <= RE_ERRORLIMIT) chkResult = createSyncObjects();

  return chkResult;
}

void MGraphics::deinitialize() {
  waitForSystemIdle();

  destroySwapChain();
  destroySyncObjects();
  destroyCommandPool();
  destroyGraphicsPipeline();
  destroyRenderPass();
  destroySurface();
  if(bRequireValidationLayers) mgrDbg->destroy(APIInstance);
  destroyLogicalDevice();
  destroyInstance();
}

void MGraphics::waitForSystemIdle() {
  vkQueueWaitIdle(logicalDevice.queues.graphics);
  vkQueueWaitIdle(logicalDevice.queues.present);
  vkDeviceWaitIdle(logicalDevice.device);
}

TResult MGraphics::createLogicalDevice(
  const CVkPhysicalDevice& physicalDeviceData) {
  if (physicalDeviceData.queueFamilyIndices.graphics.empty())
    return RE_ERROR;

  std::vector<VkDeviceQueueCreateInfo> deviceQueues;
  float queuePriority = 1.0f;

  std::set<int32_t> queueFamilySet =
    physicalDeviceData.queueFamilyIndices.getAsSet();

  for (const auto& queueFamily : queueFamilySet) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    deviceQueues.emplace_back(queueCreateInfo);
  }

  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = deviceQueues.data();
  deviceCreateInfo.queueCreateInfoCount =
    static_cast<uint32_t>(deviceQueues.size());
  deviceCreateInfo.pEnabledFeatures = &physicalDeviceData.features;
  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(core::renderer::requiredExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames =
      core::renderer::requiredExtensions.data();
  deviceCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(requiredLayers.size());
  deviceCreateInfo.ppEnabledLayerNames = requiredLayers.data();

  if (bRequireValidationLayers) {
    deviceCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(debug::validationLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = debug::validationLayers.data();
  }

  if (vkCreateDevice(physicalDeviceData.device, &deviceCreateInfo, nullptr,
                     &logicalDevice.device) != VK_SUCCESS) {
    RE_LOG(Error,
           "failed to create logical device using "
           "provided physical device: '%s'.",
           physicalDeviceData.properties.deviceName);
    return RE_ERROR;
  }

  // create graphics queue for a logical device
  vkGetDeviceQueue(logicalDevice.device,
                   physicalDevice.queueFamilyIndices.graphics[0], 0,
                   &logicalDevice.queues.graphics);

  // create present queue for a logical device
  vkGetDeviceQueue(logicalDevice.device,
                   physicalDevice.queueFamilyIndices.present[0], 0,
                   &logicalDevice.queues.present);

  RE_LOG(Log, "Successfully created logical device for '%s', handle: 0x%016llX.",
         physicalDeviceData.properties.deviceName, physicalDeviceData.device);
  return RE_OK;
}

void MGraphics::destroyLogicalDevice(VkDevice device,
                                     const VkAllocationCallbacks* pAllocator) {
  if (!device)
    device = logicalDevice.device;
  RE_LOG(Log, "Destroying logical device, handle: 0x%016llX.", device);
  vkDestroyDevice(device, pAllocator);
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