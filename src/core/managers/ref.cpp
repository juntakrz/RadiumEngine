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

void core::MRef::registerActor(ABase* pActor) {
  const std::string& name = pActor->getName();

  if (m_actorPointers.contains(name)) {
    RE_LOG(Error, "Failed to register actor '%s'.", name.c_str());
    return;
  }

  m_actorPointers[name] = pActor;
}

void core::MRef::unregisterActor(ABase* pActor) {
  const std::string& name = pActor->getName();

  if (m_actorPointers.contains(name)) {
    m_actorPointers.erase(name);
    return;
  }

  RE_LOG(Error, "Failed to unregister actor '%s'. Isn't registered with the reference manager.", name.c_str());
}

bool core::MRef::isActorRegistered(const std::string& name) {
  return m_actorPointers.contains(name);
}

ABase* core::MRef::getActor(const std::string& name) {
  return m_actorPointers.at(name);
}

void core::MRef::registerInstance(AEntity* pEntity) {
  const std::string& modelName = pEntity->getModel()->getName();
  const std::string& instanceName = pEntity->getName();

  if (!isActorRegistered(instanceName)) {
    // Make sure model entry exists, if not create one
    if (!m_sceneGraph.instances.contains(modelName)) {
      m_sceneGraph.instances[modelName] = {};
    }

    // Register new instance if it isn't present already
    if (!m_sceneGraph.instances[modelName].contains(instanceName)) {
      m_sceneGraph.instances[modelName][instanceName] = pEntity;
      m_instanceModelReferences[instanceName] = pEntity->getModel();

      registerActor(pEntity);

      return;
    }
  }

  RE_LOG(Warning, "Failed to add instance '%s' of model '%s' to scene graph. Already exists.",
    instanceName.c_str(), modelName.c_str());
}

void core::MRef::unregisterInstance(AEntity* pEntity) {
  const std::string& modelName = pEntity->getModel()->getName();
  const std::string& instanceName = pEntity->getName();

  if (isActorRegistered(instanceName)) {
    m_sceneGraph.instances[modelName].erase(instanceName);
    m_instanceModelReferences.erase(instanceName);
      
    unregisterActor(pEntity);

    if (m_sceneGraph.instances[modelName].empty()) {
      m_sceneGraph.instances.erase(modelName);
    }

    return;
  }

  RE_LOG(Warning, "Failed to remove instance '%s' of model '%s' from scene graph. It isn't registered.",
    instanceName.c_str(), modelName.c_str());
}

void core::MRef::registerCamera(ACamera* pCamera) {
  const std::string& name = pCamera->getName();

  if (!isActorRegistered(name)) {
    m_sceneGraph.cameras[name] = pCamera;
    registerActor(pCamera);

    return;
  }

  RE_LOG(Warning, "Trying to register camera '%s' with a scene graph, but it is already registered.", name.c_str());
}

void core::MRef::unregisterCamera(ACamera* pCamera) {
  const std::string& name = pCamera->getName();

  if (isActorRegistered(name)) {
    m_sceneGraph.cameras.erase(name);
    unregisterActor(pCamera);

    return;
  }

  RE_LOG(Warning, "Trying to delete camera '%s' from a scene graph, but it isn't registered.", name.c_str());
}

void core::MRef::registerLight(ALight* pLight) {
  const std::string& name = pLight->getName();

  if (!isActorRegistered(name)) {
    m_sceneGraph.lights[name] = pLight;
    registerActor(pLight);

    return;
  }

  RE_LOG(Warning, "Trying to register light '%s' with a scene graph, but it is already registered.", name.c_str());
}

void core::MRef::unregisterLight(ALight* pLight) {
  const std::string& name = pLight->getName();

  if (isActorRegistered(name)) {
    m_sceneGraph.lights.erase(name);
    unregisterActor(pLight);

    return;
  }

  RE_LOG(Warning, "Trying to delete light '%s' from a scene graph, but it isn't registered.", name.c_str());
}

void core::MRef::setSceneName(const char* name) {
  m_sceneGraph.sceneName = name;
}

const std::string& core::MRef::getSceneName() {
  return m_sceneGraph.sceneName;
}