#include "pch.h"
#include "core/core.h"
#include "core/managers/mwindow.h"
#include "core/managers/MRenderer.h"
#include "core/managers/mdebug.h"
#include "core/managers/minput.h"
#include "core/managers/mactors.h"
#include "core/managers/mscript.h"
#include "core/managers/mref.h"
#include "core/managers/mtime.h"

class core::MRenderer& core::renderer = MRenderer::get();
class core::MWindow& core::window = MWindow::get();
class core::MInput& core::input = MInput::get();
class core::MScript& core::script = MScript::get();
class core::MActors& core::actors = MActors::get();
class core::MRef& core::ref = MRef::get();
class core::MDebug& core::debug = MDebug::get();
class core::MTime& core::time = MTime::get();

void core::run() {

  // initialize engine
  RE_LOG(Log, "Radium Engine");
  RE_LOG(Log, "-------------\n");
  RE_LOG(Log, "Initializing engine core...");

  //core::graphics = &core::MRenderer::get();

  loadCoreConfig();

  RE_LOG(Log, "Creating renderer.");
  RE_CHECK(core::create());
  core::input.initialize(core::window.getWindow());

  RE_LOG(Log, "Successfully initialized engine core.");

  core::script.loadMap("default");

  RE_LOG(Log, "Launching main event loop.");

  mainEventLoop();

  stop(RE_OK);
}

void core::mainEventLoop() {
  while (!glfwWindowShouldClose(core::window.getWindow())) {
    glfwPollEvents();
    core::drawFrame();
  }
}

void core::stop(TResult cause) {
  if (cause == RE_OK) {
    RE_LOG(Log, "Shutting down on call.");

    core::destroy();
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

  json* data = core::script.jsonLoad(path, cfgName);

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

TResult core::create() {
  RE_LOG(Log, "Initializing core rendering system.");

  TResult chkResult = 0;

  // initialize Vulkan API using GLFW
  glfwInit();

  // window manager setup 
  chkResult = core::window.createWindow(config::renderWidth, config::renderHeight,
    config::appTitle, nullptr, nullptr);
  RE_CHECK(chkResult);

  // graphics manager setup (responsible for Vulkan instance and GPU management)
  RE_LOG(Log, "Initializing rendering module.");
  chkResult = core::renderer.initialize();
  RE_CHECK(chkResult);

  RE_LOG(Log, "Rendering module successfully initialized.");

  return chkResult;
}

void core::destroy() {
  core::renderer.deinitialize();
  core::window.destroyWindow();
  glfwTerminate();
}

TResult core::drawFrame() {
  TResult chkResult = RE_OK;

  chkResult = core::renderer.drawFrame();

  return chkResult;
}