#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

// linked libraries
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "ktx_read.lib")

// standard library headers
#include <algorithm>
#include <conio.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <stdarg.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// debug modules
#ifndef NDEBUG
#include <debug/renderdoc_app.h>
#include <windows.h>
#endif

// internal global definitions
#include "define.h"
#include "define_t.h"

// Vulkan API related headers
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtx/rotate_vector.hpp>
#include <GLM/gtx/quaternion.hpp>
#include <GLM/gtc/quaternion.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtx/euler_angles.hpp>
#include <GLM/gtx/matrix_interpolation.hpp>
#include <ktxvulkan.h>

// external engine modules
#include <json.hpp>

/* comments:
* 
* relies on Visual C++ 20 2022 redistributable package
* glfw3.lib - GLFW 3.3.8 built for Visual Studio 17 2022 v143 using 10.0.19041 SDK and /MD
* 
*/