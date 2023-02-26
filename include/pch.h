#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

// linked libraries
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "vulkan-1.lib")

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
#include <unordered_map>
#include <memory>
#include <random>
#include <set>
#include <stdarg.h>
#include <string>
#include <thread>
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

// external engine modules
#include <json/json.hpp>

/* comments:
* 
* relies on Visual C++ 20 2022 redistributable package
* glfw3.lib - GLFW 3.3.8 built for Visual Studio 17 2022 v143 using 10.0.19041 SDK and /MD
* 
*/