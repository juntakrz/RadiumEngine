#pragma once

#include "common.h"

class MDebug {
private:
  VkDebugUtilsMessengerEXT debugMessenger;
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

  MDebug();
  ~MDebug() {};

public:
  static MDebug& get() {
    static MDebug _sInstance;
    return _sInstance;
  }
  MDebug(const MDebug&) = delete;
  MDebug& operator=(const MDebug&) = delete;

  TResult create(VkInstance instance, VkAllocationCallbacks* pAllocator = nullptr,
                 VkDebugUtilsMessengerCreateInfoEXT* pUserData = nullptr);

  void destroy(VkInstance instance,
               VkAllocationCallbacks* pAllocator = nullptr);

  VkDebugUtilsMessengerCreateInfoEXT* info() { return &debugCreateInfo; }

private:
  static VKAPI_ATTR VkBool32 VKAPI_CALL
    callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
             VkDebugUtilsMessageTypeFlagsEXT messageType,
             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
             void* pUserData);

  VkResult createDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator);

  void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     const VkAllocationCallbacks* pAllocator);
};