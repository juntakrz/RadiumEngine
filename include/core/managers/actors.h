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
    std::unordered_map<uint32_t, std::unique_ptr<ABase>> cameras;
    std::unordered_map<uint32_t, std::unique_ptr<ALight>> lights;
    std::unordered_map<uint32_t, std::unique_ptr<APawn>> pawns;
    std::unordered_map<uint32_t, std::unique_ptr<AStatic>> statics;
  } m_actors;

  struct {
    std::vector<ABase*> pCameras;
    std::vector<ALight*> pLights;
  } m_linearActors;

  ALight* m_pSunLight = nullptr;
  int32_t m_nextActorUID = 0;

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

  // CAMERA

  ABase* createCamera(const std::string& name, RCameraInfo* cameraSettings = nullptr);
  ABase* getCamera(const std::string& name);

  // LIGHT

  ALight* createLight(const std::string& name, RLightInfo* pInfo = nullptr);
  TResult destroyLight(ALight *pLight);
  ALight* getLight(const std::string& name);
  bool setSunLight(const std::string& name);
  bool setSunLight(ALight* pLight);
  ALight* getSunLight();

  // PAWN

  APawn* createPawn(WEntityCreateInfo* pInfo);
  TResult destroyPawn(APawn* pPawn);
  APawn* getPawn(const std::string& name);
  void destroyAllPawns();

  // STATIC

  AStatic* createStatic(WEntityCreateInfo* pInfo);
  TResult destroyStatic(AStatic* pStatic);
  AStatic* getStatic(const std::string& name);
  void destroyAllStatics();

  // ENTITY

  void destroyAllEntities();
};
}  // namespace core