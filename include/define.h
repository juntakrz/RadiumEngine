#pragma once

// global library definitions
#define _CRT_SECURE_NO_WARNINGS
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// global macros
#define TEXT(x) L ## x
#define RE_LOG(l, x, ...) processMessage(l, x, __VA_ARGS__);
#define RE_CHECK(x) validate(x);

// debug build settings
#ifdef NDEBUG
constexpr bool bRequireValidationLayers = false;
#else
constexpr bool bRequireValidationLayers = true;
#endif

// generic type definitions
typedef unsigned int TResult;				// error result

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
*	CXXX		- core object, e.g. data struct or a class definition for instancing elsewhere in the code
*/