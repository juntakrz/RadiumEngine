#include "pch.h"
#include "core/objects.h"
#include "core/model/model.h"
#include "core/world/actors/entity.h"
#include "core/world/actors/pawn.h"
#include "core/world/actors/static.h"
#include "core/managers/scene.h"

core::MScene::MScene() {
  m_sceneGraph.pCameras.resize(config::scene::cameraBudget, nullptr);

  RE_LOG(Log, "Created scene graph manager.");
}

core::MScene::~MScene() {
  m_sceneActors.clear();
}

const core::MScene::WSceneGraph& core::MScene::getSceneGraph() {
  return m_sceneGraph;
}

void core::MScene::updateLightingBuffer(RLightingUBO* pLightingBuffer) {
  if (!pLightingBuffer) {
    RE_LOG(Error,
      "Couldn't update lighting uniform buffer object data. No buffer was "
      "provided.");
    return;
  }

  uint32_t lightCount = 1;
  float lightType = 1.0f;

  // Index 0 is expected to always be the directional light
  pLightingBuffer->lightColors[0] = glm::vec4(-1.0f);

  if (m_sceneGraph.pDirectionalLight) {
    pLightingBuffer->lightLocations[0] = glm::vec4(m_sceneGraph.pDirectionalLight->getWorldTranslation(), 1.0f);
    pLightingBuffer->lightColors[0] = m_sceneGraph.pDirectionalLight->getColor();
    pLightingBuffer->lightViews[0] = m_sceneGraph.pDirectionalLight->getView();
    pLightingBuffer->lightOrthoMatrix = m_sceneGraph.pDirectionalLight->getProjection();
  }

  for (const auto& pLight : m_sceneGraph.pPointLights) {
    if (pLight->getIsEnabled()) {
      pLightingBuffer->lightLocations[lightCount] = glm::vec4(pLight->getWorldTranslation(), 1.0f);
      pLightingBuffer->lightColors[lightCount] = pLight->getColor();

      ++lightCount;
    }

    // stop if max visible lights limit was reached
    if (lightCount == RE_MAXLIGHTS) break;
  }

  pLightingBuffer->lightCount = lightCount;
}

bool core::MScene::registerActor(WActor* pActor) {
  const std::string& previousName = pActor->getPreviousName();
  const std::string& name = pActor->getName();

  // Check if a target actor name isn't taken
  if (!getActor(name)) {
    // Check if the same actor with the previous name is already registered and perform renaming
    if (WActor* pActorToUnregister = getActor(previousName)) {
      if (pActorToUnregister == pActor) {
        m_actorPointers.erase(previousName);
      }
    }

    m_actorPointers[name] = pActor;
    m_actorPointersByUID[pActor->getUID()] = pActor;

    if (!m_sceneGraph.actors.contains(pActor)) {
      m_sceneGraph.actors.insert(pActor);
    }

    return true;
  }

  RE_LOG(Error, "Failed to register actor '%s', an actor with the same name is already registered.",
    name.c_str());
  return false;
}

void core::MScene::unregisterActor(WActor* pActor) {
  const std::string& name = pActor->getName();

  if (getActor(name)) {
    m_actorPointersByUID.erase(pActor->getUID());
    m_actorPointers.erase(name);

    // TODO: when actor class rework is done - remove if statement
    if (m_sceneGraph.actors.contains(pActor)) {
      m_sceneGraph.actors.erase(pActor);
    }

    return;
  }

  RE_LOG(Error, "Failed to unregister actor '%s'. It isn't registered with the reference manager.", name.c_str());
}

void core::MScene::unregisterActor(const std::string& name) {
  if (WActor* pActor = getActor(name)) {
    m_actorPointersByUID.erase(pActor->getUID());
    m_actorPointers.erase(name);
    return;
  }

  RE_LOG(Error, "Failed to unregister actor '%s'. Isn't registered with the reference manager.", name.c_str());
}

WActor* core::MScene::createActor(const std::string& name) {
  if (!getActor(name)) {
    m_sceneActors[m_nextActorUID] = std::make_unique<WActor>(m_nextActorUID);
    WActor* pNewActor = m_sceneActors[m_nextActorUID].get();
    pNewActor->setName(name);

#ifndef NDEBUG
    RE_LOG(Log, "Created actor '%s'.", name.c_str());
#endif

    ++m_nextActorUID;
    return pNewActor;
  }

#ifndef NDEBUG
  RE_LOG(Warning, "Failed to create actor '%s'. An actor with the same name already exists.",
    name.c_str());
#endif
  return getActor(name);
}

WActor* core::MScene::getActor(const std::string& name) {
  if (m_actorPointers.contains(name)) {
    return m_actorPointers[name];
  }

  return nullptr;
}

WActor* core::MScene::getActor(const int32_t UID) {
  if (m_actorPointersByUID.contains(UID)) {
    return m_actorPointersByUID[UID];
  }

  return nullptr;
}

WCameraComponent* core::MScene::getCamera(const std::string& name) {
  if (WActor* pActor = getActor(name)) {
    return pActor->getComponent<WCameraComponent>();
  }

  RE_LOG(Error, "Failed to get camera. Actor '%s' has no camera component.", name.c_str());
  return nullptr;
}

WCameraComponent* core::MScene::getCamera(const int32_t UID) {
  if (m_actorPointersByUID.contains(UID)) {
    return m_actorPointersByUID[UID]->getComponent<WCameraComponent>();
  }

  RE_LOG(Error, "Failed to get camera. Actor with UID '%d' has no camera component.", UID);
  return nullptr;
}

bool core::MScene::registerInstance(AEntity* pEntity) {
  WModel* pModel = pEntity->getModel();
  const std::string& instanceName = pEntity->getName();

  // registerActor executes first and writes a pointer to WActor, so need to make sure it did
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

bool core::MScene::unregisterInstance(AEntity* pEntity) {
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

bool core::MScene::registerCamera(WCameraComponent* pCamera) {
  // Get free camera offset index into the dynamic buffer
  for (uint32_t i = 0; i < config::scene::cameraBudget; ++i) {
    if (m_sceneGraph.pCameras[i] == nullptr) {
      pCamera->setViewBufferIndex(i);
      m_sceneGraph.pCameras[i] = pCamera;
      return true;
    }
  }

  RE_LOG(Error, "Failed to register camera of '%s', exceeded the camera budget.",
    pCamera->getOwner()->getName().c_str());
  return false;
}

bool core::MScene::unregisterCamera(WCameraComponent* pCamera) {
  for (uint32_t i = 0; i < config::scene::cameraBudget; ++i) {
    if (m_sceneGraph.pCameras[i] == pCamera) {
      pCamera->setViewBufferIndex(-1);
      m_sceneGraph.pCameras[i] = nullptr;
      return true;
    }
  }

  RE_LOG(Error, "Failed to unregister camera of '%s', is not registered.",
    pCamera->getOwner()->getName().c_str());
  return false;
}

bool core::MScene::registerPointLight(WPointLightComponent* pLight) {
  if (!pLight) {
    RE_LOG(Error, "Failed to add point light to scene manager, received nullptr.");
    return false;
  }

  // Not checked for duplicates, code using the method is expected to avoid this issue
  m_sceneGraph.pPointLights.emplace_back(pLight);
  return true;
}

bool core::MScene::unregisterPointLight(WPointLightComponent* pLight) {
  if (!pLight) {
    RE_LOG(Error, "Failed to remove point light from the scene manager, received nullptr.");
    return false;
  }

  uint32_t index = 0;
  for (auto& it : m_sceneGraph.pPointLights) {
    if (it == pLight) {
      m_sceneGraph.pPointLights.erase(m_sceneGraph.pPointLights.begin() + index);
      return true;
    }

    ++index;
  }

  RE_LOG(Error, "Failed to remove point light belonging to '%s' from the scene manager, was never registered.",
    pLight->getOwner()->getName().c_str());
  return false;
}

bool core::MScene::setDirectionalLight(WDirectLightComponent* pLight) {
  m_sceneGraph.pDirectionalLight = pLight;
  return true;
}

WDirectLightComponent* core::MScene::getDirectionalLight() {
  return m_sceneGraph.pDirectionalLight;
}

void core::MScene::setSceneName(const std::string& name) {
  bool sameNameExists = false;

  if (getActor(name)) {
    sameNameExists = true;
  }

  m_sceneGraph.sceneName = name + std::string((sameNameExists) ? "_" : "");
}

const std::string& core::MScene::getSceneName() {
  return m_sceneGraph.sceneName;
}

//
// DEPRECATED
//

APawn* core::MScene::createPawn(WEntityCreateInfo* pInfo) {
  // Model is missing
  if (!pInfo->pModel) return nullptr;

  const std::string& name = pInfo->name;

  if (getActor(name)) {
    RE_LOG(Error, "Failed to create pawn '%s'. It probably already exists.",
      name);
    return nullptr;
  };

  m_actors.pawns[m_nextActorUID] = std::make_unique<APawn>(m_nextActorUID);

  APawn* pPawn = m_actors.pawns[m_nextActorUID].get();
  pPawn->setName(name);
  pPawn->setModel(pInfo->pModel);
  pPawn->bindToRenderer();

  pPawn->setTranslation(pInfo->translation);
  pPawn->setRotation(pInfo->rotation, false);
  pPawn->setScale(pInfo->scale);

  ++m_nextActorUID;
  return pPawn;
}

TResult core::MScene::destroyPawn(APawn* pPawn) {
  const uint32_t UID = pPawn->getUID();
  pPawn->unbindFromRenderer();

  if (m_actors.pawns.contains(UID)) {
    if (pPawn->getRendererBindingIndex() > -1) {
      RE_LOG(Error,
        "Failed to destroy pawn \"%s\". It is still bound to rendering "
        "pipeline.");

      return RE_ERROR;
    }

    unregisterInstance(pPawn);
    m_actors.pawns.erase(UID);

    return RE_OK;
  }

  RE_LOG(Error, "Failed to destroy pawn at %d.", pPawn);
  return RE_ERROR;
}

APawn* core::MScene::getPawn(const std::string& name) {
  if (APawn* pPawn = getActor(name)->getAs<APawn>()) {
    return pPawn;
  }

  RE_LOG(Error, "Failed to get pawn '%s'.", name.c_str());
  return nullptr;
}

AStatic* core::MScene::createStatic(WEntityCreateInfo* pInfo) {
  // Model is missing
  if (!pInfo->pModel) return nullptr;

  const std::string& name = pInfo->name;

  if (getActor(name)) {
    RE_LOG(Error, "Failed to create static '%s'. It probably already exists.",
      name);
    return nullptr;
  };

  m_actors.statics[m_nextActorUID] = std::make_unique<AStatic>(m_nextActorUID);

  AStatic* pStatic = m_actors.statics[m_nextActorUID].get();
  pStatic->setName(name);
  pStatic->setModel(pInfo->pModel);
  pStatic->bindToRenderer();

  pStatic->setTranslation(pInfo->translation);
  pStatic->setRotation(pInfo->rotation, false);
  pStatic->setScale(pInfo->scale);

  ++m_nextActorUID;
  return pStatic;
}

TResult core::MScene::destroyStatic(AStatic* pStatic) {
  const uint32_t UID = pStatic->getUID();
  pStatic->unbindFromRenderer();

  if (m_actors.statics.contains(UID)) {
    if (pStatic->getRendererBindingIndex() > -1) {
      RE_LOG(Error,
        "Failed to destroy pawn \"%s\". It is still bound to rendering "
        "pipeline.");

      return RE_ERROR;
    }

    unregisterInstance(pStatic);
    m_actors.pawns.erase(UID);

    return RE_OK;
  }

  RE_LOG(Error, "Failed to destroy pawn at %d.", pStatic);
  return RE_ERROR;
}

AStatic* core::MScene::getStatic(const std::string& name) {
  if (AStatic* pStatic = getActor(name)->getAs<AStatic>()) {
    return pStatic;
  }

  RE_LOG(Error, "Failed to get static '%s'.", name.c_str());
  return nullptr;
}

void core::MScene::destroyAllPawns() {
  RE_LOG(Log, "Clearing all pawn buffers and allocations.");

  for (auto& it : m_actors.pawns) {
    it.second.reset();
  }

  m_actors.pawns.clear();
}