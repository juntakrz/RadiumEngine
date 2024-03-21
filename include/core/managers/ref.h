#pragma once

#include "common.h"

class ABase;
class AEntity;
class ALight;
class ACamera;
class WModel;

namespace core {

class MRef {
  friend class MGUI;

 private:
  std::unordered_map<std::string, ABase*> m_actorPointers;
  std::unordered_map<int32_t, ABase*> m_actorPointersByUID;

  struct WSceneGraph {
    std::string sceneName;

    // A map of WModel name maps that contain instances of any given model
    std::unordered_map<WModel*, std::unordered_set<AEntity*>> instances;

    std::unordered_set<ACamera*> cameras;
    std::unordered_set<ALight*> lights;
  } m_sceneGraph;

  bool m_sceneGraphRequiresUpdate = false;

 private:
  MRef();

 public:
  static MRef& get() {
    static MRef _sInstance;
    return _sInstance;
  }

  MRef(const MRef&) = delete;
  MRef& operator=(const MRef&) = delete;

  const WSceneGraph& getSceneGraph();

  // Should only be called by setName method of ABase
  bool registerActor(class ABase* pActor);

  void unregisterActor(class ABase* pActor);
  void unregisterActor(const std::string& name);

  // Can be used to check if an actor is registered, will return nullptr if not
  ABase* getActor(const std::string& name);
  ABase* getActor(const int32_t UID);

  bool registerInstance(AEntity* pEntity);
  bool unregisterInstance(AEntity* pEntity);

  bool registerCamera(ACamera* pCamera);
  bool unregisterCamera(ACamera* pCamera);

  bool registerLight(ALight* pLight);
  bool unregisterLight(ALight* pLight);

  void setSceneName(const std::string& name);
  const std::string& getSceneName();
};
}  // namespace core