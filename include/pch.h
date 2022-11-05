#pragma once

// global definitions
#include "define.h"

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
#include <memory>
#include <optional>
#include <set>
#include <stdarg.h>
#include <string>
#include <vector>

// Vulkan API related headers
#include <GLFW/glfw3.h>
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