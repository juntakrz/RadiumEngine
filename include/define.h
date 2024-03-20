#pragma once

// global library definitions
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_AVX
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

// global macros
#ifdef NDEBUG
#define TEXT(x) L ## x                // conflicts with WinAPI definitions in debug builds
#endif

#define RE_LOG(l, x, ...) util::processMessage(l, x, __VA_ARGS__)
#define RE_CHECK(x) util::validate(x)
#define ASSERT(x) \
  if (!(x)) __debugbreak();

// debug build settings
#ifdef NDEBUG
constexpr bool requireValidationLayers = false;
constexpr bool enableGPUAssistedValidation = false;
#else
constexpr bool requireValidationLayers = true;
constexpr bool enableGPUAssistedValidation = false;
#endif

// global constants
#define RE_PATH_CONFIG      TEXT("config/config.json")
#define RE_PATH_DEVCONFIG   TEXT("development/devconfig.json")
#define RE_PATH_MAP         TEXT("content/maps/")
#define RE_PATH_TEXTURES    "content/textures/"
#define RE_PATH_ANIMATIONS  "content/animations/"
#define RE_PATH_SHADERS     "content/shaders/"
#define RE_PATH_SHDRC       "development\\compileShaders_Win_x64_DEBUG.bat"

#define RE_FEXT_ANIMATIONS  ".anm"

#define RE_MAGIC_ANIMATIONS 0x4D4E41
#define RE_MAGIC_MODEL      0x4C444D

#define RE_DEFAULTTEXTURE   "default/default_baseColor.ktx2"
#define RE_WHITETEXTURE     "default/white.ktx2"
#define RE_BLACKTEXTURE     "default/black.ktx2"
#define RE_MAXTEXTURES      8
#define RE_NEARZ            0.01f
#define RE_MAXJOINTS        128u

#define MAX_FRAMES_IN_FLIGHT    3u
#define MAX_TRANSFER_BUFFERS    2u
#define RE_MAXLIGHTS            32u             // Includes directional light, always at index 0
#define RE_MAXSHADOWCASTERS     4u
#define RE_MAXTRANSPARENTLAYERS 4u
#define RE_OCCLUSIONSAMPLES     64u

// render targets
#define RTGT_PRESENT            "RT2D_Present"      // Final swapchain target
#define RTGT_DEPTH              "RT2D_Depth"        // Active camera depth render target
#define RTGT_SHADOW             "RT2D_Shadow"       // Shadow render target (depth)
#define RTGT_RAYCAST            "RT2D_Raycast"      // Map that stores instance UIDs
#define RTGT_ENVSRC             "RT2D_EnvSrc"       // Source texture for environment cubemaps
#define RTGT_PPTAA              "RT2D_TAA"          // PBR + velocity and history images
#define RTGT_PPBLOOM            "RT2D_PPBloom"      // Post processing bloom target
#define RTGT_PPAO               "RT2D_PPAO"         // Post processing ambient occlusion target
#define RTGT_PPBLUR             "RT2D_PPBlur"       // Post processing intermediate blur targets

#define RTGT_ENV                "RTCube_Env"
#define RTGT_ENVFILTER          "RTCube_EnvFilter"
#define RTGT_ENVIRRAD           "RTCube_EnvIrrad"
#define RTGT_BRDFMAP            "RT2D_EnvBRDF"
#define RTGT_NOISEMAP           "RT2D_NoiseMap"
#define RTGT_EXPOSUREMAP        "RT2D_Exposure"
#define RTGT_VELOCITYMAP        "RT2D_Velocity"
#define RTGT_PREVFRAME          "RT2D_PrevFrame"

#define RTGT_PREVDEPTH          "RT2D_PrevDepth"

#define RTGT_GPOSITION          "RT2D_GPosition"    // Fragment world space position output
#define RTGT_GDIFFUSE           "RT2D_GDiffuse"     // Diffuse output
#define RTGT_GNORMAL            "RT2D_GNormal"      // Normals output
#define RTGT_GPHYSICAL          "RT2D_GPhysical"    // Physical properties output
#define RTGT_GEMISSIVE          "RT2D_GEmissive"    // Emissive color output
#define RTGT_ABUFFER            "RT2D_ABuffer"      // Forward+ alpha output
#define RTGT_GPBR               "RT2D_GPBR"         // Render target for combined G-Buffer output
#define RTGT_APBR               "RT2D_APBR"         // Combined output of G-Buffer PBR and A-Buffer

// default materials
#define RMAT_GBUFFER            "RMat_GBuffer"
#define RMAT_MAIN               "RMat_Main"
#define RMAT_SHADOW             "RMat_Shadow"
#define RMAT_BLUR               "RMat_Blur"

// default models
#define RMDL_SKYBOX             "RMdl_SkyBox"       // default cube skybox using cubemap

// default entities
#define RACT_RENDERPLANE        "RAct_RenderPlane"  // an entity containing RMDL_RENDERPLANE

// default camera
#define RCAM_MAIN               "C_Main"
#define RCAM_ENV                "C_Environment"     // environment generating camera
#define RCAM_SUN                "C_Sun"

// default light
#define RLT_SUN                 "L_Sun"

// Error levels
#define RE_OK					      0x00		    // Success
#define RE_WARNING          0x01        // Result is acceptable, but less than ideal
#define RE_ERROR			      0x02        // Error
#define RE_CRITICAL         0x03        // Unrecoverable error
#define RE_ERRORLIMIT       RE_ERROR    // Terminate program if some error exceeds this level

// Log levels
constexpr char Log = RE_OK;
constexpr char Warning = RE_WARNING;
constexpr char Error = RE_ERROR;
constexpr char Critical = RE_CRITICAL;

// Naming convention
/*
* Exxx    - enum
*	Txxx		- type definition
*	Mxxx		- manager class, is a singleton and must be called using get() method
*	Rxxx		- renderer related class object (e.g. Vulkan API-based data structures)
* RSxxx   - (renderer) shader related object (e.g. struct for model/view/projection matrices)
* Sxxx    - generic struct object
* Wxxx    - world related class object (e.g. meshes and primitives)
*/