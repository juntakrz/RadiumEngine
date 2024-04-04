#pragma once

#include "common.h"
#include "core/objects.h"
#include "core/world/actors/pawn.h"
#include "core/world/actors/static.h"

class WActor;
class AEntity;
class ALight;
class ACamera;
class WModel;

namespace core {

class MScene {
  friend class MRenderer;
  friend class MGUI;

 private:
  std::unordered_map<uint32_t, std::unique_ptr<WActor>> m_sceneActors;
  std::unordered_map<std::string, WActor*> m_actorPointers;
  std::unordered_map<int32_t, WActor*> m_actorPointersByUID;

  struct WSceneGraph {
    std::string sceneName;

    // A map of WModel name maps that contain instances of any given model
    std::unordered_map<WModel*, std::unordered_set<AEntity*>> instances;

    std::unordered_set<WActor*> actors;
    
    std::vector<struct WCameraComponent*> pCameras;

    struct WLightComponent* pDirectionalLight = nullptr;
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

  // Should only be called by setName method of WActor
  bool registerActor(class WActor* pActor);

  void unregisterActor(class WActor* pActor);
  void unregisterActor(const std::string& name);

  WActor* createActor(const std::string& name);

  // Can be used to check if an actor is registered, will return nullptr if not
  WActor* getActor(const std::string& name);
  WActor* getActor(const int32_t UID);

  WCameraComponent* getCamera(const std::string& name);
  WCameraComponent* getCamera(const int32_t UID);

  bool registerInstance(AEntity* pEntity);
  bool unregisterInstance(AEntity* pEntity);

  bool registerCamera(WCameraComponent* pCamera);
  bool unregisterCamera(WCameraComponent* pCamera);

  bool registerPointLight(WLightComponent* pLight);
  bool unregisterPointLight(WLightComponent* pLight);

  bool setDirectionalLight(WLightComponent* pLight);
  WLightComponent* getDirectLight();

  void setSceneName(const std::string& name);
  const std::string& getSceneName();

  // DEPRECATED
  APawn* createPawn(WEntityCreateInfo* pInfo);
  TResult destroyPawn(APawn* pPawn);
  APawn* getPawn(const std::string& name);
  void destroyAllPawns();
  AStatic* createStatic(WEntityCreateInfo* pInfo);
  TResult destroyStatic(AStatic* pStatic);
  AStatic* getStatic(const std::string& name);
};
}  // namespace core