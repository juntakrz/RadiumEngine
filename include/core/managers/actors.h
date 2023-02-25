#pragma once

#include "common.h"
#include "core/world/model/mesh.h"

class ACamera;

namespace core {

class MActors {
 private:
  struct {
    std::unordered_map<std::string, std::unique_ptr<ACamera>> cameras;

  } m_actors;

 public:
  std::vector<std::unique_ptr<WMesh>> meshes;

 private:
  MActors();

 public:
  static MActors& get() {
    static MActors _sInstance;
    return _sInstance;
  }

  MActors(const MActors&) = delete;
  MActors& operator=(const MActors&) = delete;

  // CAMERA

  ACamera* createCamera(const char* name,
                        RCameraSettings* cameraSettings = nullptr);
  TResult destroyCamera(const char* name);
  ACamera* getCamera(const char* name);

  // MESH

  TResult createMesh();
  TResult destroyMesh(uint32_t index);
  void destroyAllMeshes();
};
}  // namespace core