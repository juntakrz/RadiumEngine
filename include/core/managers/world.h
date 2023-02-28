#pragma once

#include "core/world/model/model.h"

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

  // load model from file, currently only .gltf and .glb are supported
  TResult loadModelFromFile(const std::string& path, std::string name);

  // create a simple model using a chosen primitive and arguments
  TResult createModel(EWPrimitive type, std::string name, int32_t arg0,
                      int32_t arg1);
};
}  // namespace core