#pragma once

#include "common.h"

class MScript {
  using json = nlohmann::json;

  std::unordered_map<std::string, json> m_jsons;

private:
  MScript();

 public:
  static MScript& get() {
    static MScript _sInstance;
    return _sInstance;
  }

  MScript(const MScript&) = delete;
  MScript& operator=(const MScript&) = delete;

  json* jsonLoad(const wchar_t* path, const char* name) noexcept;
  json* jsonGet(const char* name);

  void jsonParseCameras(const json& cameraData) noexcept;
  void jsonParseMaterials(const json& materialData) noexcept;
  void jsonParseLights(const json& lightData) noexcept;
  void jsonParseObjects(const json& objectData) noexcept;
  void jsonParseCommands(const json& commandData) noexcept;
};