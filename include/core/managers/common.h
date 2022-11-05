#pragma once

#include "util/util.h"
#include "config.h"

// forward declarations
struct GLFWwindow;
void glfwDestroyWindow(GLFWwindow*);

// smart pointer destructor definitions
struct dtorWindow {
  void operator()(GLFWwindow* ptr) { glfwDestroyWindow(ptr); }
};