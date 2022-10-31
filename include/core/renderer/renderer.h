#pragma once

#include "core/core.h"

namespace core {
namespace renderer {

TResult create();
void destroy();

TResult drawFrame();

// mandatory extensions required by the renderer
const std::vector<const char*> requiredExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
   };
}
}  // namespace core