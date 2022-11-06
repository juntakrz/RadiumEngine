#pragma once

#define _CRT_SECURE_NO_WARNINGS

// linked libraries
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "vulkan-1.lib")

// standard library headers
#include <algorithm>
#include <conio.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <memory>
#include <set>
#include <stdarg.h>
#include <string>
#include <vector>

// internal global definitions
#include "define.h"
#include "define_t.h"

// Vulkan API related headers
#include <GLFW/glfw3.h>
#include <GLM/ext/scalar_constants.hpp>
#include <GLM/vec2.hpp>
#include <GLM/vec3.hpp>
#include <GLM/vec4.hpp>
#include <GLM/mat4x4.hpp>

// external engine modules
#include <json/json.hpp>

/* comments:
* 
* relies on Visual C++ 17 2022 redistributable package
* glfw3.lib - GLFW 3.3.8 built for Visual Studio 17 2022 v143 using 10.0.19041 SDK and /MD
* 
*/