// these are global engine variables that may be changed when compiling
#pragma once

#include "pch.h"

#define RE_ERRORLIMIT           RE_ERROR        // terminate program if some error exceeds this level
#define MAX_FRAMES_IN_FLIGHT    2u
#define MAX_TRANSFER_BUFFERS    2u

// frame buffers
#define RFB_ENV                 "FB_Env"
#define RFB_LUT                 "FB_Lut"

// render targets
#define RTGT_DEPTH              "RT2D_Depth"        // depth render target
#define RTGT_ENVSRC             "RT2D_EnvSrc"       // source texture for environment cubemaps
#define RTGT_ENVFILTER          "RTCube_EnvFilter"
#define RTGT_ENVIRRAD           "RTCube_EnvIrrad"
#define RTGT_LUTMAP             "RTCube_EnvLUT"

// default camera
#define RCAM_MAIN               "camMain"

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
// buffer overflow happens NOTE: on device 96 bytes per vertex / 4 bytes per
// index

const size_t vertexBudget = 5000000u;             // ~480 MBs for vertex data
const size_t indexBudget = 100000000u;            // ~400 MBs for index data
const size_t entityBudget = 10000u;               // ~0.6 MBs for root transformation matrices
const size_t nodeBudget = 100u * entityBudget;    // ~128 MBs for node transformation matrices
size_t getVertexBufferSize();
size_t getIndexBufferSize();
size_t getRootTransformBufferSize();
size_t getNodeTransformBufferSize();
size_t getSkinTransformBufferSize();             // ~80 MBs for joint transformation matrices
};  // namespace scene

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

const VkFormat formatLDR = VK_FORMAT_B8G8R8A8_SRGB;
const VkFormat formatHDR16 = VK_FORMAT_R16G16B16A16_SFLOAT;
const VkFormat formatHDR32 = VK_FORMAT_R32G32B32A32_SFLOAT;
const VkFormat formatLUT = VK_FORMAT_R16G16_SFLOAT;
extern VkFormat formatDepth;
const uint32_t LUTExtent = 512u;
const uint32_t envFilterExtent = 512u;
const uint32_t envIrradianceExtent = 64u;
const VkColorSpaceKHR colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
const VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
constexpr uint8_t maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;

constexpr uint8_t animationSampleRate = 30;
constexpr bool applyGLTFLeftHandedFix = false;    // currently ok for static models, but has issues with skin

}
}  // namespace core

namespace debug {
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
}

const std::vector<const char*> requiredLayers = {
    
};