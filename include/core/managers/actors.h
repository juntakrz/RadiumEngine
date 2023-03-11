#pragma once

#include "common.h"
#include "core/world/model/primitive.h"
#include "core/world/actors/pawn.h"

class ACamera;

namespace core {

class MActors {
 private:
  struct {
    std::unordered_map<std::string, std::unique_ptr<ACamera>> cameras;
    std::unordered_map<std::string, std::unique_ptr<APawn>> pawns;
  } m_actors;

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
                        RCameraInfo* cameraSettings = nullptr);
  TResult destroyCamera(const char* name);
  ACamera* getCamera(const char* name);

  // PAWN

  TResult createPawn(const char* name);
  TResult destroyPawn(const char* name);
  APawn* getPawn(const char* name);
  void destroyAllPawns();
};
}  // namespace core