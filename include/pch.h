#pragma once

// global definitions
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define RE_DEFAULTWINDOWWIDTH 1280
#define RE_DEFAULTWINDOWHEIGHT 720

// standard library headers
#include <iostream>

// common headers
#include <GLFW/glfw3.h>
#include <GLM/vec4.hpp>
#include <GLM/mat4x4.hpp>

// engine headers

/* comments:
* 
* relies on Visual C++ 17 2022 redistributable package
* glfw3.lib - GLFW 3.3.8 built for Visual Studio 17 2022 v143 using 10.0.19041 SDK and /MD
* 
*/