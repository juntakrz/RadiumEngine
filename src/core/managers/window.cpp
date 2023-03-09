#include "pch.h"
#include "core/managers/window.h"
#include "core/managers/renderer.h"
#include "core/core.h"

core::MWindow::MWindow() { RE_LOG(Log, "Creating window manager."); }

TResult core::MWindow::createWindow(uint32_t width, uint32_t height,
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

TResult core::MWindow::destroyWindow(GLFWwindow* pOtherWindow) {
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

GLFWwindow* core::MWindow::getWindow() { return pWindow.get(); }

void core::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  core::renderer.bFramebufferResized = true;
}