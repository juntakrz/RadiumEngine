#include "pch.h"
#include "core/managers/mdebug.h"

core::MDebug::MDebug() {
  RE_LOG(Log, "Creating graphics debug manager.");

  m_debugCreateInfo = {};
  m_debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  m_debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  m_debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  m_debugCreateInfo.pfnUserCallback = callback;
}

TResult core::MDebug::create(VkInstance instance, VkAllocationCallbacks* pAllocator,
                    VkDebugUtilsMessengerCreateInfoEXT* pUserData) {
  if (!bRequireValidationLayers) return RE_OK;

  RE_LOG(Log, "Initializing graphics debug subsystem.");

  m_debugCreateInfo.pUserData = pUserData;

  if(createDebugUtilsMessengerEXT(instance, &m_debugCreateInfo, pAllocator) != VK_SUCCESS)
      return RE_CRITICAL;

  return RE_OK;
}

void core::MDebug::destroy(VkInstance instance, VkAllocationCallbacks* pAllocator) {
  RE_LOG(Log, "Destroying graphics debug messenger.");
  DestroyDebugUtilsMessengerEXT(instance, pAllocator);
}

void core::MDebug::initializeRenderDoc() {
#ifndef NDEBUG
  RE_LOG(Log, "Integrating RenderDoc.");

  if (m_renderDocPath == "") {
      RE_LOG(Error,
             "Path to RenderDoc module is empty. RenderDoc will not be "
             "integrated.");
      return;
  }

  HMODULE renderDocModule = LoadLibraryA(m_renderDocPath.c_str());

  if (renderDocModule == NULL) {
      DWORD lastError = GetLastError();
      RE_LOG(Error,
             "Failed to get handle for 'renderdoc.dll'. WinAPI error code: %u, "
             "path to module: '%s'.",
             lastError, m_renderDocPath.c_str());
      return;
  }

  pRENDERDOC_GetAPI getAPI =
      (pRENDERDOC_GetAPI)GetProcAddress(renderDocModule, "RENDERDOC_GetAPI");

  if (getAPI == nullptr) {
      RE_LOG(Error,
             "Failed to retrieve RenderDoc GetAPI entry point from "
             "'renderdoc.dll'");
      return;
  }

  if (getAPI(eRENDERDOC_API_Version_1_6_0, (void**)&m_pRenderDoc) != 1) {
      RE_LOG(Error, "Failed to set RenderDoc API 1.6.0 up.");
      return;
  }

#endif
}

void core::MDebug::setRenderDocModulePath(const std::string& path) {
#ifndef NDEBUG
  m_renderDocPath = path + "renderdoc.dll";
  RE_LOG(Log, "RenderDoc full module path is set to: '%s'.",
         m_renderDocPath.c_str());
#endif
}

VKAPI_ATTR VkBool32 VKAPI_CALL
core::MDebug::callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData) {
  std::cerr << "Vulkan validation layer: " << pCallbackData->pMessage << "\n";
  return VK_FALSE;
}

VkResult core::MDebug::createDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pm_debugCreateInfo,
    const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pm_debugCreateInfo, pAllocator, &m_debugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void core::MDebug::DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, m_debugMessenger, pAllocator);
  }
}