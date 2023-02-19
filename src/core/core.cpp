#include "pch.h"
#include "core/core.h"
#include "core/renderer/renderer.h"
#include "core/managers/mwindow.h"
#include "core/managers/mgraphics.h"
#include "core/managers/mdebug.h"
#include "core/managers/minput.h"
#include "core/managers/mmodel.h"
#include "core/managers/mscript.h"
#include "core/managers/mref.h"

class core::MGraphics* core::graphics = nullptr;

void core::run() {

  // initialize engine
  RE_LOG(Log, "Radium Engine");
  RE_LOG(Log, "-------------\n");
  RE_LOG(Log, "Initializing engine core...");

  core::graphics = &core::MGraphics::get();

  loadCoreConfig();

  RE_LOG(Log, "Creating renderer.");
  RE_CHECK(core::renderer::create());
  MInput::get().initialize(MWindow::get().window());

  RE_LOG(Log, "Successfully initialized engine core.");

  MScript::get().loadMap("default");

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
  using json = nlohmann::json;
  uint32_t resolution[2] = { config::renderWidth, config::renderHeight };
  uint8_t requirements = 3;
  const char* cfgName = "cfgCore";

  json* data = MScript::get().jsonLoad(path, cfgName);

  if (data->contains("core")) {
    const auto& coreData = data->at("core");
    if (coreData.contains("resolution")) {
      coreData.at("resolution").get_to(resolution);
      config::renderWidth = resolution[0];
      config::renderHeight = resolution[1];
      --requirements;
    }

    if (coreData.contains("devMode")) {
      coreData.at("devMode").get_to(config::bDevMode);
      --requirements;
    }

    --requirements;
  }

  if (!requirements) {
    RE_LOG(Log, "Successfully loaded core config at '%s'.",
           toString(path).c_str());
    return;
  }

  RE_LOG(Error, "Core configuration file seems to be corrupted.");
}
