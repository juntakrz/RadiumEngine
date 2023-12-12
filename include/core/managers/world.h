#pragma once

#include "core/model/model.h"

namespace core {

class MWorld {
 private:
  std::unordered_map<std::string, std::unique_ptr<WModel>> m_models;

  MWorld();

 public:
  static MWorld& get() {
    static MWorld _sInstance;
    return _sInstance;
  }

  // load model from file, .gltf and .glb models are supported
  TResult loadModelFromFile(const std::string& path, const char* name,
                            const bool tryExtractAnimations = false,
                            const float framerate = 15.0f,
                            const float speed = 1.0f);

  // create a simple model using a chosen primitive and arguments
  TResult createModel(EPrimitiveType type, std::string name, int32_t arg0,
                      int32_t arg1);

  WModel* getModel(const char* name);

  // call after objects (pawns, statics) using models are already destroyed
  void destroyAllModels();
};
}  // namespace core