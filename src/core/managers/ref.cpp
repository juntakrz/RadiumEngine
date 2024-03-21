#include "pch.h"
#include "core/objects.h"
#include "core/model/model.h"
#include "core/world/actors/camera.h"
#include "core/world/actors/entity.h"
#include "core/world/actors/light.h"
#include "core/managers/ref.h"

core::MRef::MRef() { RE_LOG(Log, "Created scene graph manager."); }

const core::MRef::WSceneGraph& core::MRef::getSceneGraph() {
  return m_sceneGraph;
}

bool core::MRef::registerActor(ABase* pActor) {
  const std::string& previousName = pActor->getPreviousName();
  const std::string& name = pActor->getName();

  // Check if a target actor name isn't taken
  if (!getActor(name)) {
    // Check if the same actor with the previous name is already registered and perform renaming
    if (ABase* pActorToUnregister = getActor(previousName)) {
      if (pActorToUnregister == pActor) {
        m_actorPointers.erase(previousName);
      }
    }

    m_actorPointers[name] = pActor;
    m_actorPointersByUID[pActor->getUID()] = pActor;
    return true;
  }

  RE_LOG(Error, "Failed to register actor '%s', an actor with the same name is already registered.",
    name.c_str());
  return false;
}

void core::MRef::unregisterActor(ABase* pActor) {
  const std::string& name = pActor->getName();

  if (getActor(name)) {
    m_actorPointersByUID.erase(pActor->getUID());
    m_actorPointers.erase(name);
    return;
  }

  RE_LOG(Error, "Failed to unregister actor '%s'. Isn't registered with the reference manager.", name.c_str());
}

void core::MRef::unregisterActor(const std::string& name) {
  if (ABase* pActor = getActor(name)) {
    m_actorPointersByUID.erase(pActor->getUID());
    m_actorPointers.erase(name);
    return;
  }

  RE_LOG(Error, "Failed to unregister actor '%s'. Isn't registered with the reference manager.", name.c_str());
}

ABase* core::MRef::getActor(const std::string& name) {
  if (m_actorPointers.contains(name)) {
    return m_actorPointers[name];
  }

  return nullptr;
}

ABase* core::MRef::getActor(const int32_t UID) {
  if (m_actorPointersByUID.contains(UID)) {
    return m_actorPointersByUID[UID];
  }

  return nullptr;
}

bool core::MRef::registerInstance(AEntity* pEntity) {
  WModel* pModel = pEntity->getModel();
  const std::string& instanceName = pEntity->getName();

  // registerActor executes first and writes a pointer to ABase, so need to make sure it did
  if (getActor(instanceName) == pEntity) {
    // Make sure model entry exists, if not create one
    if (!m_sceneGraph.instances.contains(pModel)) {
      m_sceneGraph.instances[pModel] = {};
    }

    // Register new instance if it isn't present already
    if (!m_sceneGraph.instances[pModel].contains(pEntity)) {
      m_sceneGraph.instances[pModel].insert(pEntity);

      return true;
    }
  }

  RE_LOG(Warning, "Failed to add instance '%s' of model '%s' to scene graph. Already exists.",
    instanceName.c_str(), pModel->getName().c_str());

  return false;
}

bool core::MRef::unregisterInstance(AEntity* pEntity) {
  WModel* pModel = pEntity->getModel();
  const std::string& instanceName = pEntity->getName();

  if (getActor(instanceName) == pEntity) {
    m_sceneGraph.instances[pModel].erase(pEntity);
    unregisterActor(pEntity);

    if (m_sceneGraph.instances[pModel].empty()) {
      m_sceneGraph.instances.erase(pModel);
    }

    return true;
  }

  RE_LOG(Warning, "Failed to remove instance '%s' of model '%s' from scene graph. It isn't registered.",
    instanceName.c_str(), pModel->getName().c_str());
  return false;
}

bool core::MRef::registerCamera(ACamera* pCamera) {
  const std::string& name = pCamera->getName();

  if (!m_sceneGraph.cameras.contains(pCamera)) {
    m_sceneGraph.cameras.insert(pCamera);
    return true;
  }

  RE_LOG(Warning, "Trying to register camera '%s' with a scene graph, but it is already registered.", name.c_str());
  return false;
}

bool core::MRef::unregisterCamera(ACamera* pCamera) {
  const std::string& name = pCamera->getName();

  if (getActor(name)) {
    m_sceneGraph.cameras.erase(pCamera);
    unregisterActor(pCamera);

    return true;
  }

  RE_LOG(Warning, "Trying to delete camera '%s' from a scene graph, but it isn't registered.", name.c_str());
  return false;
}

bool core::MRef::registerLight(ALight* pLight) {
  const std::string& name = pLight->getName();

  if (!m_sceneGraph.lights.contains(pLight)) {
    m_sceneGraph.lights.insert(pLight);
    return true;
  }

  RE_LOG(Warning, "Trying to register light '%s' with a scene graph, but it is already registered.", name.c_str());
  return false;
}

bool core::MRef::unregisterLight(ALight* pLight) {
  const std::string& name = pLight->getName();

  if (getActor(name)) {
    m_sceneGraph.lights.erase(pLight);
    unregisterActor(pLight);

    return true;
  }

  RE_LOG(Warning, "Trying to delete light '%s' from a scene graph, but it isn't registered.", name.c_str());
  return false;
}

void core::MRef::setSceneName(const std::string& name) {
  bool sameNameExists = false;

  if (getActor(name)) {
    sameNameExists = true;
  }

  m_sceneGraph.sceneName = name + std::string((sameNameExists) ? "_" : "");
}

const std::string& core::MRef::getSceneName() {
  return m_sceneGraph.sceneName;
}