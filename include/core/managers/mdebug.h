#pragma once

#include "common.h"

namespace core {

  class MDebug {
  private:
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT m_debugCreateInfo;

    std::string m_renderDocPath = "";       // retrieved on launch from the development configuration

    #ifndef NDEBUG
    RENDERDOC_API_1_6_0* m_pRenderDoc = nullptr;
    #endif

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

    VkDebugUtilsMessengerCreateInfoEXT* info() { return &m_debugCreateInfo; }
    
    // RenderDoc (methods will not execute if NDEBUG is set)
    void initializeRenderDoc();
    void setRenderDocModulePath(const std::string& path);

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
}