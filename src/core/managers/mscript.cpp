#include "pch.h"
#include "core/managers/mscript.h"

using json = nlohmann::json;

MScript::MScript() { RE_LOG(Log, "Preparing script manager."); }

json* MScript::jsonLoad(const wchar_t* path, const char* name) noexcept {
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
  RE_LOG(Log, "Loaded '%s' from '%s'.", name, path);
  return &m_jsons.at(name);
}

json* MScript::jsonGet(const char* name) {
  if (m_jsons.find(name) != m_jsons.end()) {
    return &m_jsons.at(name);
  }

  RE_LOG(Error, "Failed to get JSON at '%s'. Not found.", name);
  return nullptr;
}

void MScript::jsonParseCameras(const json& cameraData) noexcept { /*
  for (const auto& it : cameraData.at("cameras")) {
    // create camera if it doesn't exist
    if (it.contains("name")) {
      std::string name = it.at("name");
      float pos[3] = {0.0f, 0.0f, 0.0f};
      float rotation[2] = {0.0f, 0.0f};

      void* ptr = mgrGfx->addCamera(name);

      // set camera position
      if (it.contains("position")) {
        it.at("position").get_to(pos);
      }

      // set camera rotation
      DF.Camera(name)->SetPos({pos[0], pos[1], pos[2]});

      if (it.contains("rotation")) {
        it.at("rotation").get_to(rotation);
      }

      DF.Camera(name)->SetRotation(rotation[0], rotation[1]);

      // set camera mode
      /* vars:
              orthographic: x = width, y = height, z = nearZ, w = farZ
              perspective:  x = FOV, y = aspect ratio, z = nearZ, w = farZ
      */
                                                                  /*
      if (it.contains("mode")) {
        float vars[4];
        it.at("mode").at("variables").get_to(vars);

        if (it.at("mode").at("view") == "orthographic") {
          DF.Camera(name)->SetAsOrthographic(vars[0], vars[1], vars[2],
                                             vars[3]);
        }

        if (it.at("mode").at("view") == "perspective") {
          DF.Camera(name)->SetAsPerspective(vars[0], vars[1], vars[2], vars[3]);
        }
      }

      DF.RefM->Add(name, ptr, DFRefMgr::Type::Camera);
    }
  }*/
}

void MScript::jsonParseMaterials(const json& materialData) noexcept {}

void MScript::jsonParseLights(const json& lightData) noexcept {}

void MScript::jsonParseObjects(const json& objectData) noexcept {}

void MScript::jsonParseCommands(const json& commandData) noexcept {}
