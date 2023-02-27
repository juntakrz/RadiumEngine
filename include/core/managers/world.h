#pragma once

#include "core/world/model/model.h"

namespace tinygltf {
class Model;
class Node;
}

namespace core {

class MWorld {
 private:
  std::unordered_map<std::string, std::unique_ptr<WModel>> m_models;

  MWorld();
  void processModelNode(WModel* pNewModel, const tinygltf::Model& model,
                        const tinygltf::Node& node);

 public:
  static MWorld& get() {
    static MWorld _sInstance;
    return _sInstance;
  }

  // load model from file, optionally give it a custom name
  TResult loadModelFromFile(const std::string& path, std::string name = "");
};
}  // namespace core