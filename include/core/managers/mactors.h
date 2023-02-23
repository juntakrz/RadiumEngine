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

  // actors controlled by the player input
  std::vector<ABase*> m_inputActors;

 public:
  std::vector<std::unique_ptr<WMesh>> meshes;

 private:
  MActors();

  void bindDefaultMethods();

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

  // ACTOR INTERFACE

  void controlActor(ABase* pActor);
  void controlCamera(const char* name);

  void freeActor(ABase* pActor);

  // standard movement methods which control currently selected actors
  void translateForward();
  void translateBack();
  void translateLeft();
  void translateRight();
  void translateUp();
  void translateDown();
};
}  // namespace core