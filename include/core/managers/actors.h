#pragma once

#include "common.h"
#include "core/model/primitive.h"
#include "core/world/actors/light.h"
#include "core/world/actors/pawn.h"
#include "core/world/actors/static.h"

class ACamera;

namespace core {

class MActors {
  friend class MRenderer;

 private:
  struct {
    std::unordered_map<std::string, std::unique_ptr<ACamera>> cameras;
    std::unordered_map<std::string, std::unique_ptr<ALight>> lights;
    std::unordered_map<std::string, std::unique_ptr<APawn>> pawns;
    std::unordered_map<std::string, std::unique_ptr<AStatic>> statics;
  } m_actors;

  struct {
    std::vector<ACamera*> pCameras;
    std::vector<ALight*> pLights;
    std::vector<APawn*> pPawns;
    std::vector<AStatic*> pStatics;
  } m_linearActors;

  ALight* m_pSunLight = nullptr;

 private:
  MActors();

  // method should be called only by renderer
  void updateLightingUBO(RLightingUBO* pLightingBuffer);

 public:
  static MActors& get() {
    static MActors _sInstance;
    return _sInstance;
  }

  MActors(const MActors&) = delete;
  MActors& operator=(const MActors&) = delete;

  void initialize();

  // CAMERA

  ACamera* createCamera(const char* name,
                        RCameraInfo* cameraSettings = nullptr);
  TResult destroyCamera(const char* name);
  ACamera* getCamera(const char* name);

  // LIGHT

  ALight* createLight(const char* name, RLightInfo* pInfo = nullptr);
  TResult destroyLight(const char* name);
  ALight* getLight(const char* name);
  bool setSunLight(const char* name);
  bool setSunLight(ALight* pLight);
  ALight* getSunLight();

  // PAWN

  APawn* createPawn(const char* name);
  TResult destroyPawn(const char* name);
  APawn* getPawn(const char* name);
  void destroyAllPawns();

  // STATIC

  AStatic* createStatic(const char* name);
  TResult destroyStatic(const char* name);
  AStatic* getStatic(const char* name);
  void destroyAllStatics();

  // ENTITY

  void destroyAllEntities();
};
}  // namespace core