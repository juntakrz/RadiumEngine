#pragma once

#include "common.h"
#include "core/world/mesh/mesh.h"

class ACamera;

namespace core {

  class MActors {
 private:
  struct {
    std::unordered_map<std::string, std::unique_ptr<ACamera>> cameras;
  } m_actors;

  MActors();

 public:
  static MActors& get() {
    static MActors _sInstance;
    return _sInstance;
  }

  MActors(const MActors&) = delete;
  MActors& operator=(const MActors&) = delete;

  ACamera* createCamera(const char* name, RCameraSettings* cameraSettings = nullptr);
  TResult destroyCamera(const char* name);
  ACamera* getCamera(const char* name);

  TResult createMesh();
  TResult destroyMesh(uint32_t index);
  void destroyAllMeshes();

  std::vector<std::unique_ptr<WMesh>> meshes;
};
}