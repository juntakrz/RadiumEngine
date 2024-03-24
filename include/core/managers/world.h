#pragma once

#include "core/world/components/component.h"
#include "core/model/model.h"

namespace core {

class MWorld {
 private:
  std::unordered_map<std::string, std::unique_ptr<WModel>> m_models;
  std::unordered_map<std::type_index, std::unique_ptr<WComponent>> m_componentRegistry;

  MWorld();

 public:
  static MWorld& get() {
    static MWorld _sInstance;
    return _sInstance;
  }

  void initialize();

  // Load model from file, .gltf and .glb models are supported
  TResult loadModelFromFile(const std::string& path, std::string name,
                            const WModelConfigInfo* pConfigInfo = nullptr);

  // Create a simple model using a chosen primitive and arguments
  TResult createModel(EPrimitiveType type, std::string name, int32_t arg0,
                      int32_t arg1);

  WModel* getModel(const char* name);

  // Call after objects (pawns, statics) using models are already destroyed
  void destroyAllModels();

  // Retrieve a pointer to the component template from the registry
  template<typename T>
  T* getComponent() {
    if (m_componentRegistry.contains(typeid(T))) {
      return dynamic_cast<T*>(m_componentRegistry[typeid(T)].get());
    }

    return nullptr;
  }
};
}  // namespace core