#pragma once

// global library definitions
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// global macros
#define TEXT(x) L ## x
#define RE_LOG(l, x, ...) processMessage(l, x, __VA_ARGS__)
#define RE_CHECK(x) validate(x)
#define ASSERT(x) if (!x) __debugbreak;

// debug build settings
#ifdef NDEBUG
constexpr bool bRequireValidationLayers = false;
#else
constexpr bool bRequireValidationLayers = true;
#endif

// global constants
#define RE_PATH_CONFIG      TEXT("config/config.json")
constexpr double RE_PI64 =  3.14159265358979323846264338327950288;
constexpr float RE_PI =     (float)RE_PI64;
constexpr float RE_PIDIV2 = RE_PI / 2.0f;
constexpr float RE_2PI =    RE_PI * 2.0f;

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
*	TXXX		- type definition
*	MXXX		- manager class, is a singleton and must be called using get() method
*	RXXX		- renderer related class object (e.g. Vulkan API-based data structures)
* WXXX    - world related class object (e.g. meshes and primitives)
*/