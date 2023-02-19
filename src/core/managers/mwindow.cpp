#include "pch.h"
#include "core/managers/mwindow.h"
#include "core/managers/mgraphics.h"
#include "core/core.h"

MWindow::MWindow() { RE_LOG(Log, "Creating window manager."); }

TResult MWindow::createWindow(const uint32_t& width, const uint32_t& height,
                              const char* title, GLFWmonitor* monitor,
                              GLFWwindow* share) {
  RE_LOG(Log, "Initializing window subsystem.");
  RE_LOG(Log, "creating window '%s' (%d x %d).", title, width, height);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  pWindow = std::unique_ptr<GLFWwindow, dtorWindow>(
    glfwCreateWindow(width, height, title, monitor, share));

  if (!pWindow) {
    RE_LOG(Critical, "failed to create window '%s'.", title);
    return RE_CRITICAL;
  }

  glfwSetFramebufferSizeCallback(pWindow.get(), framebufferResizeCallback);

  RE_LOG(Log, "window '%s' successfully created.", title);
  return RE_OK;
}

TResult MWindow::destroyWindow(GLFWwindow* pOtherWindow) {
  RE_LOG(Log, "Closing the window.");

  if (pWindow) {
    glfwDestroyWindow(pOtherWindow);
    return RE_OK;
  }

  if (!pOtherWindow) return RE_ERROR;

  glfwDestroyWindow(pWindow.get());
  pWindow.reset();
  return RE_OK;
}

GLFWwindow* MWindow::window() { return pWindow.get(); }

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  core::graphics->bFramebufferResized = true;
}