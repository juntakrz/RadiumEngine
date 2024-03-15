// these are global engine variables that may be changed when compiling
#pragma once

#include "pch.h"

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
extern uint32_t shadowResolution;
extern uint32_t shadowCascades;
extern float maxAnisotropy;
extern uint32_t ambientOcclusionMode;

// scene buffer values
namespace scene {
// expected unique vertices and indices
// that scene vertex and index buffers will be allocated for
// TODO: set these through map configuration
//
// TODO: if ever needed - allocate new scene vertex and index buffers when
// buffer overflow happens NOTE: on device 96 bytes per vertex / 4 bytes per
// index

constexpr size_t vertexBudget = 10000000u;                  // Unique, reusable vertex data
constexpr size_t indexBudget = 100000000u;                  // Unique, reusable index data
constexpr size_t entityBudget = 10000u;                     // Unique entity data (e.g. unique model and skin matrices)
constexpr size_t nodeBudget = 1000u * RE_MAXJOINTS;         // Maximum unique fully animated meshes with max joints
constexpr size_t uniquePrimitiveBudget = 16384u;            // Maximum unique primitives (used to generate draw indirect commands)
constexpr size_t cameraBudget = 64u;

extern uint32_t sampledImageBudget;
extern uint32_t storageImageBudget;
const uint32_t requestedStorageImageBudget = 128u;      // Max number of image views available to compute

extern uint32_t cameraBlockSize;
extern uint32_t nodeBlockSize;
extern uint32_t skinBlockSize;

size_t getVertexBufferSize();
size_t getIndexBufferSize();
size_t getRootTransformBufferSize();
size_t getNodeTransformBufferSize();
size_t getSkinTransformBufferSize();                    // ~82 MBs for joint transformation matrices
size_t getMaxCameraCount();
};  // namespace scene

float getAspectRatio();
};  // namespace config

namespace core {
namespace vulkan {

// Vulkan API version
const uint32_t APIversion = VK_API_VERSION_1_3;

// mandatory extensions required by the renderer
const std::vector<const char*> requiredExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
};

const VkFormat formatLDR = VK_FORMAT_B8G8R8A8_SRGB;
const VkFormat formatHDR16 = VK_FORMAT_R16G16B16A16_SFLOAT;
const VkFormat formatHDR32 = VK_FORMAT_R32G32B32A32_SFLOAT;
const VkFormat formatLUT = VK_FORMAT_R16G16_SFLOAT;
extern VkFormat formatDepth;
extern VkFormat formatShadow;
const uint32_t LUTExtent = 512u;
const uint32_t envFilterExtent = 512u;
const uint32_t envIrradianceExtent = 64u;
const VkColorSpaceKHR colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
const VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
constexpr uint8_t maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;
extern VkDeviceSize minUniformBufferAlignment;
extern VkDeviceSize descriptorBufferOffsetAlignment;
constexpr bool applyGLTFLeftHandedFix = false;
constexpr uint32_t maxSampler2DDescriptors = 4096u; // amount of allowed variable index descriptors
constexpr uint8_t haltonSequenceCount = 16u;
}
}  // namespace core

namespace debug {
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
}

const std::vector<const char*> requiredLayers = {
    
};