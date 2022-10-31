// these are global engine variables that may be changed when compiling
#pragma once

#include "pch.h"

#define RE_ERRORLIMIT           RE_ERROR        // terminate program if some error exceeds this level
#define MAX_FRAMES_IN_FLIGHT    2u

struct {
  const char* appTitle = "Radium Engine";
  const char* engineTitle = "Radium Engine";
  uint32_t renderWidth = 1280;
  uint32_t renderHeight = 720;
} settings;

namespace core {
namespace renderer {
// desired swap chain settings, will be set if supported by the physical device
const VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
const VkColorSpaceKHR colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
const VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
constexpr uint8_t maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;
}
}  // namespace core

namespace debug {
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
}

const std::vector<const char*> requiredLayers = {
    
};