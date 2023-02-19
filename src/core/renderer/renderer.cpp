#include "pch.h"
#include "core/renderer/renderer.h"
#include "core/managers/mwindow.h"
#include "core/managers/mgraphics.h"
#include "core/managers/mdebug.h"

TResult core::renderer::create() {
  RE_LOG(Log, "Initializing core rendering system.");

  TResult chkResult = 0;

  // initialize Vulkan API using GLFW
  glfwInit();

  // window manager setup 
  chkResult = MWindow::get().createWindow(config::renderWidth, config::renderHeight,
                                   config::appTitle, nullptr, nullptr);
  RE_CHECK(chkResult);

  // graphics manager setup (responsible for Vulkan instance and GPU management)
  RE_LOG(Log, "Initializing graphics subsystem.");
  chkResult = core::graphics->initialize();
  RE_CHECK(chkResult);

  RE_LOG(Log, "Rendering system successfully initialized.");

  return chkResult;
}

void core::renderer::destroy() {
  core::graphics->deinitialize();
  MWindow::get().destroyWindow();
  glfwTerminate();
}

TResult core::renderer::drawFrame () {
  TResult chkResult = RE_OK;

  chkResult = core::graphics->drawFrame();

  return chkResult;
}