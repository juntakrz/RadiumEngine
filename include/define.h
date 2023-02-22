#pragma once

// global library definitions
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SSE42
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

// global macros
#ifdef NDEBUG
#define TEXT(x) L ## x                // conflicts with WinAPI definitions in debug builds
#endif

#define RE_LOG(l, x, ...) processMessage(l, x, __VA_ARGS__)
#define RE_CHECK(x) validate(x)
#define ASSERT(x) \
  if (!(x)) __debugbreak();

// debug build settings
#ifdef NDEBUG
constexpr bool bRequireValidationLayers = false;
#else
constexpr bool bRequireValidationLayers = true;
#endif

// global constants
#define RE_PATH_CONFIG      TEXT("config/config.json")
#define RE_PATH_DEVCONFIG   TEXT("development/devconfig.json")
#define RE_PATH_MAP         TEXT("content/maps/")
#define RE_PATH_TEXTURES    TEXT("content/textures/")
#define RE_PATH_SHDRC       "development\\compileShaders_Win_x64_DEBUG.bat"
#define RE_MAXTEXTURES      6

// error levels
#define RE_OK					      0x00		   // success
#define RE_WARNING          0x01       // result is acceptable, but less than ideal
#define RE_ERROR			      0x02       // error
#define RE_CRITICAL         0x03       // unrecoverable error

// log levels
constexpr char Log =        RE_OK;
constexpr char Warning =    RE_WARNING;
constexpr char Error =      RE_ERROR;
constexpr char Critical =   RE_CRITICAL;

// naming conventions
/*
* Exxx    - enum
*	Txxx		- type definition
*	Mxxx		- manager class, is a singleton and must be called using get() method
*	Rxxx		- renderer related class object (e.g. Vulkan API-based data structures)
* RSxxx   - (renderer) shader related object (e.g. struct for model/view/projection matrices)
* Wxxx    - world related class object (e.g. meshes and primitives)
*/