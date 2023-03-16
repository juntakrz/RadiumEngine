// these are global engine variables that may be changed when compiling
#pragma once

#include "pch.h"

#define RE_ERRORLIMIT           RE_ERROR        // terminate program if some error exceeds this level
#define MAX_FRAMES_IN_FLIGHT    2u
#define MAX_TRANSFER_BUFFERS    2u

struct RVertex;

namespace config {
extern const char* appTitle;
extern const char* engineTitle;
extern uint32_t renderWidth;
extern uint32_t renderHeight;
extern float viewDistance;                      // aka FarZ
extern float FOV;
extern bool bDevMode;
extern float pitchLimit;                        // camera pitch limit

// scene buffer values
namespace scene {
  // expected unique vertices and indices
  // that scene vertex and index buffers will be allocated for
  // TODO: set these through map configuration
  //
  // TODO: if ever needed - allocate new scene vertex and index buffers when
  // buffer overflow happens NOTE: a million of unique vertices and indices are
  // ~120 MBs of memory together

  const size_t vertexBudget = 5000000u;    // ~580 MBs for vertex data
  const size_t indexBudget = 100000000u;   // ~400 MBs for index data
  size_t getVertexBufferSize();
  size_t getIndexBufferSize();
};

float getAspectRatio();
};  // namespace config

namespace core {
namespace vulkan {

// Vulkan API version
const uint32_t APIversion = VK_API_VERSION_1_3;

// mandatory extensions required by the renderer
const std::vector<const char*> requiredExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// desired swap chain settings, will be set if supported by the physical device
const VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
const VkColorSpaceKHR colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
const VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
constexpr uint8_t maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;

constexpr bool bFlipViewPortY = true;    // for OpenGL / Direct3D vertex coordinate compatibility
}
}  // namespace core

namespace debug {
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
}

const std::vector<const char*> requiredLayers = {
    
};