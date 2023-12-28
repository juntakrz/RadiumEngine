// these are global engine variables that may be changed when compiling
#pragma once

#include "pch.h"

#define RE_ERRORLIMIT           RE_ERROR        // terminate program if some error exceeds this level
#define MAX_FRAMES_IN_FLIGHT    2u
#define MAX_TRANSFER_BUFFERS    2u
#define RE_MAXLIGHTS            32              // includes directional light, always at index 0

// frame buffers
#define RFB_DEFERRED            "FB_Deferred"       // generate G-buffer textures
#define RFB_PBR                 "FB_PBR"            // combine PBR result of G-buffer textures with skybox
#define RFB_PRESENT             "FB_Present"        // convert 16 bit float result to 8 bit presentable image
#define RFB_ENV                 "FB_Env"
#define RFB_LUT                 "FB_Lut"

// render targets
#define RTGT_DEPTH              "RT2D_Depth"        // active camera depth render target
#define RTGT_SHADOW             "RT2D_Shadow"       // shadow render target
#define RTGT_ENVSRC             "RT2D_EnvSrc"       // source texture for environment cubemaps

#define RTGT_ENVFILTER          "RTCube_EnvFilter"
#define RTGT_ENVIRRAD           "RTCube_EnvIrrad"
#define RTGT_LUTMAP             "RTCube_EnvLUT"

#define RTGT_GPBR               "RT2D_GPBR"         // render target for combined G-Buffer PBR output
#define RTGT_GPOSITION          "RT2D_GPosition"    // fragment world space position output
#define RTGT_GDIFFUSE           "RT2D_GDiffuse"     // diffuse output
#define RTGT_GNORMAL            "RT2D_GNormal"      // normals output
#define RTGT_GPHYSICAL          "RT2D_GPhysical"    // physical properties output
#define RTGT_GEMISSIVE          "RT2D_GEmissive"    // emissive color output

// default materials
#define RMAT_GBUFFER            "RMat_GBuffer"      // material storing G-buffer targets as textures
#define RMAT_PRESENT            "RMat_Present"

// default models
#define RMDL_RENDERPLANE        "RMdl_RenderPlane"  // a screen-wide plane for rendering to
#define RMDL_SKYBOX             "RMdl_SkyBox"       // default cube skybox using cubemap

// default entities
#define RACT_RENDERPLANE        "RAct_RenderPlane"  // an entity containing RMDL_RENDERPLANE

// default camera
#define RCAM_MAIN               "C_Main"
#define RCAM_SUN                "C_Sun"

// default light
#define RLT_SUN                 "L_Sun"

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

const size_t vertexBudget = 5000000u;                   // ~480 MBs for vertex data
const size_t indexBudget = 100000000u;                  // ~400 MBs for index data
const size_t entityBudget = 10000u;                     // ~0.6 MBs for root transformation matrices
const size_t nodeBudget = RE_MAXJOINTS * entityBudget;  // ~164 MBs for node transformation matrices
size_t getVertexBufferSize();
size_t getIndexBufferSize();
size_t getRootTransformBufferSize();
size_t getNodeTransformBufferSize();
size_t getSkinTransformBufferSize();                    // ~82 MBs for joint transformation matrices
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
extern VkDeviceSize minBufferAlignment;
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