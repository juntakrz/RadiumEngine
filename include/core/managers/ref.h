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
  std::unordered_map<std::string, WModel*> m_instanceModelReferences; // Models by instance name

  struct WSceneGraph {
    std::string sceneName;

    // A map of WModel name maps that contain instances of any given model
    std::unordered_map<std::string, std::unordered_map<std::string, AEntity*>> instances;

    std::unordered_map<std::string, ACamera*> cameras;
    std::unordered_map<std::string, ALight*> lights;
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

  void registerActor(class ABase* pActor);
  void unregisterActor(class ABase* pActor);
  bool isActorRegistered(const std::string& name);
  ABase* getActor(const std::string& name);

  void registerInstance(AEntity* pEntity);
  void unregisterInstance(AEntity* pEntity);

  void registerCamera(ACamera* pCamera);
  void unregisterCamera(ACamera* pCamera);

  void registerLight(ALight* pLight);
  void unregisterLight(ALight* pLight);

  void setSceneName(const char* name);
  const std::string& getSceneName();
};
}  // namespace core