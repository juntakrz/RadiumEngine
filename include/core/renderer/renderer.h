#pragma once

#include "core/core.h"

namespace core {
namespace renderer {

TResult create();
void destroy();

TResult drawFrame();

// Vulkan API version
const uint32_t APIversion = VK_API_VERSION_1_2;

// mandatory extensions required by the renderer
const std::vector<const char*> requiredExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
   };
}
}  // namespace core