#include "pch.h"
#include "core/core.h"
#include "core/managers/window.h"
#include "core/managers/renderer.h"
#include "core/managers/debug.h"
#include "core/managers/input.h"
#include "core/managers/actors.h"
#include "core/managers/animations.h"
#include "core/managers/player.h"
#include "core/managers/script.h"
#include "core/managers/ref.h"
#include "core/managers/time.h"
#include "core/managers/resources.h"
#include "core/managers/world.h"

class core::MRenderer& core::renderer = MRenderer::get();
class core::MWindow& core::window = MWindow::get();
class core::MInput& core::input = MInput::get();
class core::MScript& core::script = MScript::get();
class core::MActors& core::actors = MActors::get();
class core::MDebug& core::debug = MDebug::get();
class core::MResources& core::resources = MResources::get();
class core::MPlayer& core::player = MPlayer::get();
class core::MRef& core::ref = MRef::get();
class core::MTime& core::time = MTime::get();
class core::MWorld& core::world = MWorld::get();
class core::MAnimations& core::animations = MAnimations::get();

void core::run() {

  loadCoreConfig();

#ifndef NDEBUG
  loadDevelopmentConfig();
  core::debug.compileDebugShaders();
  core::debug.initializeRenderDoc();
#endif

  RE_LOG(Log, "Creating renderer.");
  RE_CHECK(core::create());

  RE_LOG(Log, "Successfully initialized engine core.");

  core::script.loadMap("default");

  // remove this after loadMap improvements -------- //
  RSamplerInfo samplerInfo{};
  core::resources.loadTexture("skyboxCubemap.ktx2", &samplerInfo);
  //core::resources.loadTexture("papermill.ktx", &samplerInfo);

  RMaterialInfo materialInfo{};
  materialInfo.name = "skybox";
  materialInfo.passFlags = EDynamicRenderingPass::Skybox | EDynamicRenderingPass::EnvSkybox;
  materialInfo.textures.baseColor = "skyboxCubemap.ktx2";
  //materialInfo.glowColor = glm::vec4(0.5);
  core::resources.createMaterial(&materialInfo);

  // create map models
  core::world.createModel(EPrimitiveType::Sphere, "mdlSphere", 16, false);
  core::world.createModel(EPrimitiveType::Sphere, "mdlSphere1", 4, false);
  core::world.createModel(EPrimitiveType::Cube, "mdlBox1", 1, false);

  core::world.getModel(RMDL_SKYBOX)->setPrimitiveMaterial(0, 0, "skybox");

  WModelConfigInfo modelConfigInfo{};
  modelConfigInfo.animationLoadMode = EAnimationLoadMode::ExtractToManager;

  core::world.loadModelFromFile("content/models/wc3guy/scene.gltf", "mdlGuy", &modelConfigInfo);
  core::world.loadModelFromFile("content/models/castle/scene.gltf", "mdlCastle");
  core::world.loadModelFromFile("content/models/tree/scene.gltf", "mdlTree");
  core::world.loadModelFromFile("content/models/grass_02/scene.gltf", "mdlGrass");
  //
  
  // create entities
  materialInfo = RMaterialInfo{};
  materialInfo.name = "RMat_Light0";
  materialInfo.glowColor = glm::vec4(3.5f, 2.4f, 1.0f, 0.0f);
  core::resources.createMaterial(&materialInfo);

  materialInfo = RMaterialInfo{};
  materialInfo.name = "RMat_Light1";
  materialInfo.glowColor = glm::vec4(1.0f, 1.0f, 3.0f, 0.0f);
  core::resources.createMaterial(&materialInfo);

  
  APawn* pPawn = core::actors.createPawn("sphere0");
  pPawn->setModel(core::world.getModel("mdlSphere"));
  pPawn->setLocation(1.0f, -0.8f, 2.0f);
  pPawn->setScale(0.18f);
  pPawn->getModel()->setPrimitiveMaterial(0, 0, "RMat_Light0");
  pPawn->bindToRenderer();

  pPawn = core::actors.createPawn("sphere1");
  pPawn->setModel(core::world.getModel("mdlSphere"));
  pPawn->setLocation(1.5f, 1.4f, 3.3f);
  pPawn->setScale(0.1f);
  pPawn->bindToRenderer();
  pPawn->setInstancePrimitiveMaterial(0, 0, "RMat_Light1");

  AStatic* pStatic = core::actors.createStatic("Skybox");
  pStatic->setModel(core::world.getModel(RMDL_SKYBOX));
  pStatic->bindToRenderer();
  
  pStatic = core::actors.createStatic("Static01");
  pStatic->setModel(core::world.getModel("mdlGuy"));
  pStatic->bindToRenderer();
  pStatic->setLocation(0.0f, -1.0f, -0.3f);
  pStatic->setRotation({0.0f, 1.0f, 0.0f}, glm::radians(100.0f));
  pStatic->setScale(0.32f);

  //pStatic->getModel()->bindAnimation("Windy day");
  //pStatic->getModel()->playAnimation("Windy day");

  //core::animations.loadAnimation("Idle");
  
  pStatic->getModel()->bindAnimation("Idle");   // TODO: not used, fix or deprecate
  pStatic->playAnimation("Idle");

  pStatic = core::actors.createStatic("Static02");
  pStatic->setModel(core::world.getModel("mdlGuy"));
  pStatic->bindToRenderer();
  pStatic->setLocation(2.5f, -1.38f, 2.1f);
  pStatic->setRotation({ 0.0f, 1.0f, 0.0f }, glm::radians(180.0f));
  pStatic->setScale(0.32f);
  pStatic->playAnimation("SwordAndShieldIdle");

  pStatic = core::actors.createStatic("Static03");
  pStatic->setModel(core::world.getModel("mdlGuy"));
  pStatic->bindToRenderer();
  pStatic->setLocation(1.0f, -1.1f, 0.4f);
  pStatic->setRotation({ 0.0f, 1.0f, 0.0f }, glm::radians(-150.0f));
  pStatic->setScale(0.32f);
  pStatic->playAnimation("SwordAndShieldIdle", 1.1f, true, true);

  pStatic = core::actors.createStatic("StaticCastle");
  pStatic->setModel(core::world.getModel("mdlCastle"));
  pStatic->bindToRenderer();
  pStatic->setLocation(0.0f, 6.17f, 12.0f);
  pStatic->setRotation({-0.5f, -0.4f, 0.0f});
  pStatic->setScale(2.0f);

  pStatic = core::actors.createStatic("StaticTree0");
  pStatic->setModel(core::world.getModel("mdlTree"));
  pStatic->bindToRenderer();
  pStatic->setLocation(1.0f, -1.35f, -0.8f);
  pStatic->setRotation({ 0.0f, 0.0f, 1.5f });
  pStatic->setScale({ 1.0f, 1.25f, 1.0f });

  pStatic = core::actors.createStatic("StaticGrass0");
  pStatic->setModel(core::world.getModel("mdlGrass"));
  pStatic->bindToRenderer();
  pStatic->setLocation(-0.12f, -0.7f, -0.3f);
  pStatic->setRotation({ -1.707f, 0.0f, 0.0f });
  pStatic->setScale(0.4f);

  pStatic = core::actors.createStatic("StaticGrass1");
  pStatic->setModel(core::world.getModel("mdlGrass"));
  pStatic->bindToRenderer();
  pStatic->setLocation(0.3f, -0.68f, -0.4f);
  pStatic->setRotation({ -1.707f, 0.0f, 0.0f });
  pStatic->setScale(0.4f);

  //core::animations.saveAnimation("SwordAndShieldIdle", "SwordAndShieldIdle");
  //
  //core::renderer.renderView.generateEnvironmentMapsImmediate = true;
  core::renderer.renderView.generateEnvironmentMaps = true;
  // ---------------------------- */

  RE_LOG(Log, "Launching main event loop.");

  mainEventLoop();

  stop(RE_OK);
}

void core::mainEventLoop() {
  while (!glfwWindowShouldClose(core::window.getWindow())) {
    glfwPollEvents();
    core::input.scanInput();
    core::drawFrame();
  }
}

void core::stop(TResult cause) {
  if (cause == RE_OK) {
    RE_LOG(Log, "Shutting down on call.");

    core::destroy();

    RE_LOG(Log, "Exiting program normally.");
  }
  else {
    RE_LOG(
        Log,
        "Shutting down due to error. Press any key to terminate the program.");
    _getch();
  }
  
  exit(cause);
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

  core::resources.initialize();
  core::world.initialize();

  core::renderer.renderInitFrame();

  core::input.initialize(core::window.getWindow());
  core::player.initialize();

  
#ifndef NDEBUG
  core::renderer.debug_initialize();
#endif

  return chkResult;
}

void core::destroy() {
  core::renderer.deinitialize();
  core::window.destroyWindow();
  glfwTerminate();
}

void core::drawFrame() {
  core::renderer.renderFrame();
}

void core::loadCoreConfig(const wchar_t* path) {
  using json = nlohmann::json;
  uint32_t resolution[2] = {config::renderWidth, config::renderHeight};
  uint8_t requirements = 3;
  const char* cfgName = "cfgCore";

  json* data = core::script.jsonLoad(path, cfgName);

  RE_LOG(Log, "Reading base application settings.");

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

  RE_LOG(Log, "Applying graphics settings.");

  if (data->contains("graphics")) {
    const auto& graphicsData = data->at("graphics");

    if (graphicsData.contains("viewDistance")) {
      graphicsData.at("viewDistance").get_to(config::viewDistance);
    }

    if (graphicsData.contains("FOV")) {
      graphicsData.at("FOV").get_to(config::FOV);
    }
  }

  RE_LOG(Log, "Parsing input bindings.");

  if (data->contains("keyboard")) {
    const auto& keySet = data->at("keyboard");
    
    for (const auto& it : keySet.items()) {
      std::string bindInput;
      it.value().get_to(bindInput);
      core::input.setInputBinding(it.key(), bindInput);
    }
  }

  if (data->contains("mouse")) {
    const auto& mouseSet = data->at("mouse");

    for (const auto& it : mouseSet.items()) {
      // read mouse axes here
    }
  }

  if (!requirements) {
    return;
  }

  RE_LOG(Error, "Core configuration file seems to be corrupted.");
}

void core::loadDevelopmentConfig(const wchar_t* path) {
  #ifndef NDEBUG
  using json = nlohmann::json;
  const char* cfgName = "cfgDevelopment";

  json* data = core::script.jsonLoad(path, cfgName);

  // look for renderdoc settings
  if (data->contains("renderdoc")) {
    const auto& coreData = data->at("renderdoc");
    if (coreData.contains("enabled")) {
      coreData.at("enabled").get_to(core::debug.getRenderDoc().bEnabled);
    }
    
    if (core::debug.getRenderDoc().bEnabled) {
      if (coreData.contains("path")) {
        std::string path;
        coreData.at("path").get_to(path);
        core::debug.getRenderDoc().path = path + "renderdoc.dll";
      }
      if (coreData.contains("showOverlay")) {
        coreData.at("showOverlay")
            .get_to(core::debug.getRenderDoc().bEnableOverlay);
      }
    }
  }

  #endif
}