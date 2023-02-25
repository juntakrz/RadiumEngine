#include "pch.h"
#include "core/core.h"
#include "core/managers/script.h"
#include "core/managers/renderer.h"
#include "core/managers/actors.h"
#include "core/managers/player.h"
#include "core/managers/ref.h"
#include "util/util.h"

using json = nlohmann::json;

core::MScript::MScript() { RE_LOG(Log, "Created script manager."); }

TResult core::MScript::loadMap(const char* mapName) {
  // map structure constants
  const std::wstring mapPath = RE_PATH_MAP + toWString(mapName) + TEXT(".map/");
  const std::wstring initPath = mapPath + L"init.json";
  const std::wstring camPath = mapPath + L"cameras.json";
  const std::wstring matPath = mapPath + L"materials.json";
  const std::wstring lightPath = mapPath + L"lights.json";
  const std::wstring objPath = mapPath + L"objects.json";
  const std::wstring commPath = mapPath + L"commands.json";

  // parse cameras
  jsonParseCameras(jsonLoad(camPath.c_str(), "cameraData"));

  return RE_OK;
}

json* core::MScript::jsonLoad(const wchar_t* path, const char* name) noexcept {
  if (!path) {
    RE_LOG(Error, "jsonLoad received empty path.");
    return nullptr;
  }

  if (!m_jsons.try_emplace(name).second) {
    RE_LOG(Warning, "JSON is already loaded at '%s', overwriting.", name);
  }

  m_jsons.at(name) = json{};
  json* out_j = &m_jsons.at(name);

  std::ifstream fStream(path);
  out_j->clear();

  if (!fStream.good()) {
    RE_LOG(Error, "Failed to read JSON file at '%s'.", path);
    return nullptr;
  }

  fStream >> *out_j;
  
  std::string str = toString(path);
  RE_LOG(Log, "Loaded '%s' from '%s'.", name, str.c_str());
  return &m_jsons.at(name);
}

json* core::MScript::jsonGet(const char* name) {
  if (m_jsons.find(name) != m_jsons.end()) {
    return &m_jsons.at(name);
  }

  RE_LOG(Error, "Failed to get JSON at '%s'. Not found.", name);
  return nullptr;
}

TResult core::MScript::jsonRemove(const char* jsonId) {
  if (m_jsons.find(jsonId) != m_jsons.end()) {
    m_jsons.erase(jsonId);
    return RE_OK;
  }

  RE_LOG(Error, "failed to remove jsonId '%s' - does not exist.", jsonId);
  return RE_ERROR;
}

void core::MScript::clearAllScripts() { m_jsons.clear(); }

void core::MScript::jsonParseCameras(const json* cameraData) noexcept {
  
  if (!cameraData) {
    RE_LOG(Error, "no camera data was provided for map parser.");
    return;
  }

  std::string activatedCamera = "";

  for (const auto& it : cameraData->at("cameras")) {
    // create camera if it doesn't exist
    if (it.contains("name")) {
      std::string name = it.at("name");

      if (name == "$ACTIVE") {
        if (it.contains("activate")) {
          activatedCamera = it.at("activate");
          continue;
        }
      }

      float pos[3] = {0.0f, 0.0f, 0.0f};
      float rotation[3] = {0.0f, 0.0f, 0.0f};
      float upVector[3] = {0.0f, 1.0f, 0.0f};

      ACamera* newCamera = core::actors.createCamera(name.c_str(), nullptr);

      // set camera position
      if (it.contains("position")) {
        it.at("position").get_to(pos);
      }

      if (it.contains("rotation")) {
        it.at("rotation").get_to(rotation);
      }

      if (it.contains("upVector")) {
        it.at("upVector").get_to(upVector);
      }

      newCamera->setPos({pos[0], pos[1], pos[2]});
      newCamera->setRotation({rotation[0], rotation[1], rotation[2]});
      newCamera->setUpVector({upVector[0], upVector[1], upVector[2]});

      // set camera mode
      /* vars:
              orthographic: x = width, y = height, z = nearZ, w = farZ
              perspective:  x = FOV, y = aspect ratio, z = nearZ, w = farZ
      */

      if (it.contains("mode")) {
        float vars[4];
        it.at("mode").at("variables").get_to(vars);

        if (it.at("mode").at("view") == "orthographic") {
          //DF.Camera(name)->SetAsOrthographic(vars[0], vars[1], vars[2],
            //                                 vars[3]);
        }

        if (it.at("mode").at("view") == "perspective") {
          newCamera->setPerspective(vars[0], vars[1], vars[2], vars[3]);
        }
      }

      core::ref.registerActor(name.c_str(), newCamera);
    }
  }

  if (activatedCamera != "") {
    core::renderer.setCamera(activatedCamera.c_str());

    // TODO: make this a separate thing in a map config
    core::player.controlActor(core::renderer.getCamera());
  }
}

void core::MScript::jsonParseMaterials(const json* materialData) noexcept {}

void core::MScript::jsonParseLights(const json* lightData) noexcept {}

void core::MScript::jsonParseObjects(const json* objectData) noexcept {}

void core::MScript::jsonParseCommands(const json* commandData) noexcept {}
