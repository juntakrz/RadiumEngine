#pragma once

#include "common.h"

namespace core {

  class MDebug {
  private:
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT m_debugCreateInfo;

#ifndef NDEBUG
    struct DebugRenderDoc {
      bool bEnabled = false;
      std::string path = "";
      bool bEnableOverlay = true;
      HMODULE hRenderDocModule = NULL;
      RENDERDOC_API_1_6_0* pAPI = nullptr;
    } m_renderdoc;
#endif

    MDebug();
    ~MDebug();

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

    void compileDebugShaders();
    
    // RenderDoc (methods will not execute if NDEBUG is set)
    void initializeRenderDoc();
    void enableRenderDocOverlay(bool bEnable);
    DebugRenderDoc& getRenderDoc();

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