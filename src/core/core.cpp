#include "pch.h"
#include "core/core.h"
#include "core/renderer/renderer.h"
#include "core/managers/MWindow.h"
#include "core/managers/MGraphics.h"
#include "core/managers/MDebug.h"

class MGraphics* mgrGfx = nullptr;
class MWindow* mgrWnd = nullptr;
class MDebug* mgrDbg = nullptr;

void core::run() {
  // initialize engine
  RE_LOG(Log, "Radium Engine");
  RE_LOG(Log, "-------------\n");
  RE_LOG(Log, "Initializing engine core...");

  // create and register managers
  RE_LOG(Log, "Registering managers.");
  mgrWnd = &MWindow::get();
  mgrGfx = &MGraphics::get();
  mgrDbg = &MDebug::get();

  RE_CHECK(core::renderer::create());

  RE_LOG(Log, "Successfully initialized engine core.");
  RE_LOG(Log, "Launching main event loop.");

  mainEventLoop();

  stop(RE_OK);
}

void core::mainEventLoop() {
  while (!glfwWindowShouldClose(MWindow::get().window())) {
    glfwPollEvents();
    core::renderer::drawFrame();
  }
}

void core::stop(TResult cause) {
  if (cause == RE_OK) {
    RE_LOG(Log, "Shutting down on call.");

    core::renderer::destroy();
  }
  else {
    RE_LOG(
        Log,
        "Shutting down due to error. Press any key to terminate the program.");
    _getch();
  }
  
  exit(cause);
}
