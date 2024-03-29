#include "pch.h"
#include "core/core.h"
#include "core/managers/input.h"
#include "core/managers/ref.h"
#include "core/managers/renderer.h"
#include "core/managers/world.h"
#include "core/world/actors/camera.h"
#include "core/managers/actors.h"

core::MActors::MActors() {
  RE_LOG(Log, "Creating actors manager.");
}

void core::MActors::updateLightingUBO(RLightingUBO* pLightingBuffer) {
  if (!pLightingBuffer) {
    RE_LOG(Error,
           "Couldn't update lighting uniform buffer object data. No buffer was "
           "provided.");
    return;
  }

  uint32_t lightCount = 1;
  float lightType = 1.0f;

  // Index 0 is expected to always be the directional light
  if (m_lights.pDirectLight) {
    ABase* pActor = m_lights.pDirectLight->getOwner();
    pLightingBuffer->lightLocations[0] = glm::vec4(pActor->getTranslation(), 1.0f);
    pLightingBuffer->lightColors[0] = m_lights.pDirectLight->getColor();
    //pLightingBuffer->lightViews[0] = m_pSunLight->getView();
    //pLightingBuffer->lightOrthoMatrix = m_pSunLight->getProjection();
  }

  for (const auto& pLight : m_lights.pPointLights) {
    if (pLight->getIsEnabled() && pLight->getLightType() != ELightType::Directional) {
      pLightingBuffer->lightLocations[lightCount] = glm::vec4(pLight->getWorldTranslation(), 1.0f);
      pLightingBuffer->lightColors[lightCount] = pLight->getColor();

      ++lightCount;
    }

    // stop if max visible lights limit was reached
    if (lightCount == RE_MAXLIGHTS) break;
  }

  pLightingBuffer->lightCount = lightCount;
}

ABase* core::MActors::createCamera(const std::string& name, RCameraInfo* pInfo) {
  if (!core::ref.getActor(name)) {
    m_sceneActors[m_nextActorUID] = std::make_unique<ABase>(m_nextActorUID);
    ABase* pCameraActor = m_sceneActors[m_nextActorUID].get();

    pCameraActor->setName(name);
    WCameraComponent* pComponent = pCameraActor->addComponent<WCameraComponent>();

    const RCameraInfo& cameraInfo = (pInfo) ? *pInfo : RCameraInfo();
    pComponent->setCameraParameters(
      cameraInfo.projectionMode, cameraInfo.FOV, cameraInfo.aspectRatio, cameraInfo.viewDistance);

    // get free camera offset index into the dynamic buffer
    uint32_t index = 0;
    
    for (const auto& it : m_linearActors.pCameras) {
      if (it->getComponent<WCameraComponent>()->getViewBufferIndex() != index) {
        break;
      }

      ++index;
    }

    pComponent->setViewBufferIndex(index);
    m_linearActors.pCameras.emplace_back(pCameraActor);

    core::ref.registerActor(pCameraActor);

#ifndef NDEBUG
    RE_LOG(Log, "Created camera '%s'.", name.c_str());
#endif

    ++m_nextActorUID;
    return pCameraActor;
  }

#ifndef NDEBUG
  RE_LOG(Warning, "Failed to create camera '%s'. Probably already exists.",
         name.c_str());
#endif
  return getCamera(name);
}

ABase* core::MActors::getCamera(const std::string& name) {
  if (ABase* pCamera = core::ref.getActor(name)) {
    return pCamera;
  }

  return nullptr;
}

void core::MActors::setDirectLight(WLightComponent* pDirectLight) {
  if (pDirectLight && pDirectLight->getLightType() == ELightType::Directional) {
    m_lights.pDirectLight = pDirectLight;
    return;
  }

  RE_LOG(Error, "Failed to set direct light caster.");
}

WLightComponent* core::MActors::getDirectLight() {
  return m_lights.pDirectLight;
}

void core::MActors::addPointLight(WLightComponent* pPointLight) {
  if (!pPointLight) {
    RE_LOG(Error, "Failed to add point light to scene manager, received nullptr.");
    return;
  }

  // Not checked for duplicates
  m_lights.pPointLights.emplace_back(pPointLight);
}

void core::MActors::removePointLight(WLightComponent* pPointLight) {
  if (!pPointLight) {
    RE_LOG(Error, "Failed to remove point light from the scene manager, received nullptr.");
    return;
  }

  uint32_t index = 0;
  for (auto& it : m_lights.pPointLights) {
    if (it == pPointLight) {
      m_lights.pPointLights.erase(m_lights.pPointLights.begin() + index);
      return;
    }

    ++index;
  }

  RE_LOG(Error, "Failed to remove point light belonging to '%s' from the scene manager, was never registered.",
    pPointLight->getOwner()->getName().c_str());
}

APawn* core::MActors::createPawn(WEntityCreateInfo* pInfo) {
  // Model is missing
  if (!pInfo->pModel) return nullptr;

  const std::string& name = pInfo->name;

  if (core::ref.getActor(name)) {
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

TResult core::MActors::destroyPawn(APawn* pPawn) {
  const uint32_t UID = pPawn->getUID();
  pPawn->unbindFromRenderer();

  if (m_actors.pawns.contains(UID)) {
    if (pPawn->getRendererBindingIndex() > -1) {
      RE_LOG(Error,
        "Failed to destroy pawn \"%s\". It is still bound to rendering "
        "pipeline.");

      return RE_ERROR;
    }

    core::ref.unregisterInstance(pPawn);
    m_actors.pawns.erase(UID);

    return RE_OK;
  }

  RE_LOG(Error, "Failed to destroy pawn at %d.", pPawn);
  return RE_ERROR;
}

APawn* core::MActors::getPawn(const std::string& name) {
  if (APawn* pPawn = core::ref.getActor(name)->getAs<APawn>()) {
    return pPawn;
  }

  RE_LOG(Error, "Failed to get pawn '%s'.", name.c_str());
  return nullptr;
}

AStatic* core::MActors::createStatic(WEntityCreateInfo *pInfo) {
  // Model is missing
  if (!pInfo->pModel) return nullptr;

  const std::string& name = pInfo->name;

  if (core::ref.getActor(name)) {
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

TResult core::MActors::destroyStatic(AStatic *pStatic) {
  const uint32_t UID = pStatic->getUID();
  pStatic->unbindFromRenderer();

  if (m_actors.statics.contains(UID)) {
    if (pStatic->getRendererBindingIndex() > -1) {
      RE_LOG(Error,
        "Failed to destroy pawn \"%s\". It is still bound to rendering "
        "pipeline.");

      return RE_ERROR;
    }

    core::ref.unregisterInstance(pStatic);
    m_actors.pawns.erase(UID);

    return RE_OK;
  }

  RE_LOG(Error, "Failed to destroy pawn at %d.", pStatic);
  return RE_ERROR;
}

AStatic* core::MActors::getStatic(const std::string& name) {
  if (AStatic* pStatic = core::ref.getActor(name)->getAs<AStatic>()) {
    return pStatic;
  }

  RE_LOG(Error, "Failed to get static '%s'.", name.c_str());
  return nullptr;
}

void core::MActors::destroyAllStatics() {}

void core::MActors::destroyAllEntities() {}

void core::MActors::destroyAllPawns() {
  RE_LOG(Log, "Clearing all pawn buffers and allocations.");

  for (auto& it : m_actors.pawns) {
    it.second.reset();
  }

  m_actors.pawns.clear();
}