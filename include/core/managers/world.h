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

  // load model from file, optionally give it a custom name
  void loadModelFromFile(const char* path, const char* name = "");
};
}  // namespace core