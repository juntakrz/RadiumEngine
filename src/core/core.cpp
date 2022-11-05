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

  loadCoreConfig();

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

void core::loadCoreConfig(const wchar_t* path) {
  json data;
  uint32_t resolution[2] = { config::renderWidth, config::renderHeight };

  if (jsonLoad(path, &data) != RE_OK) {
    RE_LOG(Error,
           "Failed to load core configuration file. Default settings will be "
           "used.");
    return;
  };

  if (data.contains("core")) {
    const auto& coreData = data.at("core");
    if (coreData.contains("resolution")) {
      coreData.at("resolution").get_to(resolution);
      config::renderWidth = resolution[0];
      config::renderHeight = resolution[1];
    }

    RE_LOG(Log, "Successfully loaded core config at '%s'.",
           wstrToStr(path).c_str());
    return;
  }

  RE_LOG(Error, "Core configuration file seems to be corrupted.");
}
