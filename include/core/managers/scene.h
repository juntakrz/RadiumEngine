#pragma once

#include "common.h"
#include "core/objects.h"
#include "core/world/actors/pawn.h"
#include "core/world/actors/static.h"

class ABase;
class AEntity;
class ALight;
class ACamera;
class WModel;

namespace core {

class MScene {
  friend class MRenderer;
  friend class MGUI;

 private:
  std::unordered_map<uint32_t, std::unique_ptr<ABase>> m_sceneActors;
  std::unordered_map<std::string, ABase*> m_actorPointers;
  std::unordered_map<int32_t, ABase*> m_actorPointersByUID;

  struct WSceneGraph {
    std::string sceneName;

    // A map of WModel name maps that contain instances of any given model
    std::unordered_map<WModel*, std::unordered_set<AEntity*>> instances;

    std::unordered_set<ABase*> actors;
    
    std::vector<struct WCameraComponent*> pCameras;

    struct WLightComponent* pDirectLight = nullptr;
    std::vector<struct WLightComponent*> pPointLights;
  } m_sceneGraph;

  int32_t m_nextActorUID = 0;

  // DEPRECATED
  struct {
    std::unordered_map<uint32_t, std::unique_ptr<APawn>> pawns;
    std::unordered_map<uint32_t, std::unique_ptr<AStatic>> statics;
  } m_actors;
  //

 private:
  MScene();

 public:
  static MScene& get() {
    static MScene _sInstance;
    return _sInstance;
  }

  MScene(const MScene&) = delete;
  MScene& operator=(const MScene&) = delete;

  const WSceneGraph& getSceneGraph();

  void updateLightingBuffer(RLightingUBO* pLightingBuffer);

  // Should only be called by setName method of ABase
  bool registerActor(class ABase* pActor);

  void unregisterActor(class ABase* pActor);
  void unregisterActor(const std::string& name);

  // Can be used to check if an actor is registered, will return nullptr if not
  ABase* getActor(const std::string& name);
  ABase* getActor(const int32_t UID);

  bool registerInstance(AEntity* pEntity);
  bool unregisterInstance(AEntity* pEntity);

  bool registerCamera(WCameraComponent* pCamera);
  bool unregisterCamera(WCameraComponent* pCamera);

  bool registerPointLight(WLightComponent* pLight);
  bool unregisterPointLight(WLightComponent* pLight);

  bool setDirectLight(WLightComponent* pLight);
  WLightComponent* getDirectLight();

  void setSceneName(const std::string& name);
  const std::string& getSceneName();

  // DEPRECATED
  ABase* createCamera(const std::string& name, RCameraInfo* cameraSettings = nullptr);
  ABase* getCamera(const std::string& name);
  APawn* createPawn(WEntityCreateInfo* pInfo);
  TResult destroyPawn(APawn* pPawn);
  APawn* getPawn(const std::string& name);
  void destroyAllPawns();
  AStatic* createStatic(WEntityCreateInfo* pInfo);
  TResult destroyStatic(AStatic* pStatic);
  AStatic* getStatic(const std::string& name);
};
}  // namespace core