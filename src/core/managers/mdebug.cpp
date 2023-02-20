#include "pch.h"
#include "core/managers/mdebug.h"

core::MDebug::MDebug() {
  RE_LOG(Log, "Creating graphics debug manager.");

  debugCreateInfo = {};
  debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugCreateInfo.pfnUserCallback = callback;
}

TResult core::MDebug::create(VkInstance instance, VkAllocationCallbacks* pAllocator,
                    VkDebugUtilsMessengerCreateInfoEXT* pUserData) {
  if (!bRequireValidationLayers) return RE_OK;

  RE_LOG(Log, "Initializing graphics debug subsystem.");

  debugCreateInfo.pUserData = pUserData;

  if(createDebugUtilsMessengerEXT(instance, &debugCreateInfo, pAllocator) != VK_SUCCESS)
      return RE_CRITICAL;

  return RE_OK;
}

void core::MDebug::destroy(VkInstance instance, VkAllocationCallbacks* pAllocator) {
  RE_LOG(Log, "Destroying graphics debug messenger.");
  DestroyDebugUtilsMessengerEXT(instance, pAllocator);
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
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pdebugCreateInfo,
    const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pdebugCreateInfo, pAllocator, &debugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void core::MDebug::DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}