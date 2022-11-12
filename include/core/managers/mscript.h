#pragma once

#include "common.h"

namespace core {

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

    TResult loadMap(const char* mapName);

    json* jsonLoad(const wchar_t* path, const char* jsonId) noexcept;
    json* jsonGet(const char* jsonId);
    TResult jsonRemove(const char* jsonId);

    // clear all loaded JSONs from memory
    void clearAllScripts();

    void jsonParseCameras(const json* cameraData) noexcept;
    void jsonParseMaterials(const json* materialData) noexcept;
    void jsonParseLights(const json* lightData) noexcept;
    void jsonParseObjects(const json* objectData) noexcept;
    void jsonParseCommands(const json* commandData) noexcept;
  };
}