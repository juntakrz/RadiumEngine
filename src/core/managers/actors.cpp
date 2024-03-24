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

  // Index 0 is expected to be the directional 'sun' light
  if (m_pSunLight) {
    pLightingBuffer->lightLocations[0] = glm::vec4(m_pSunLight->getTranslation(), 1.0f);
    pLightingBuffer->lightColors[0] = glm::vec4(m_pSunLight->getLightColor(), m_pSunLight->getLightIntensity());
    pLightingBuffer->lightViews[0] = m_pSunLight->getView();
    pLightingBuffer->lightOrthoMatrix = m_pSunLight->getProjection();
  }

  // Currently all other lights are expected to be point lights
  for (const auto& pLight : m_linearActors.pLights) {
    if (pLight->isVisible() && pLight != m_pSunLight) {
      pLightingBuffer->lightLocations[lightCount] = glm::vec4(pLight->getTranslation(), 1.0f);
      pLightingBuffer->lightColors[lightCount] = glm::vec4(pLight->getLightColor(), pLight->getLightIntensity());

      ++lightCount;
    }

    // stop if max visible lights limit was reached
    if (lightCount == RE_MAXLIGHTS) break;
  }

  pLightingBuffer->lightCount = lightCount;
}

ACamera* core::MActors::createCamera(const std::string& name, RCameraInfo* cameraSettings) {
  if (!core::ref.getActor(name)) {
    m_actors.cameras[m_nextActorUID] = std::make_unique<ACamera>(m_nextActorUID);
    ACamera* pCamera = m_actors.cameras[m_nextActorUID].get();

    pCamera->setName(name);

    if (cameraSettings) {
      pCamera->setPerspective(
          cameraSettings->FOV, cameraSettings->aspectRatio,
          cameraSettings->nearZ, cameraSettings->farZ);
    } else {
      pCamera->setPerspective(
          config::FOV, config::getAspectRatio(),
          RE_NEARZ, config::viewDistance);
    }

    // get free camera offset index into the dynamic buffer
    uint32_t index = 0;
    
    for (const auto& it : m_linearActors.pCameras) {
      if (it->getViewBufferIndex() != index) {
        break;
      }

      ++index;
    }

    pCamera->setViewBufferIndex(index);
    m_linearActors.pCameras.emplace_back(pCamera);

    core::ref.registerCamera(pCamera);

#ifndef NDEBUG
    RE_LOG(Log, "Created camera '%s'.", name.c_str());
#endif

    ++m_nextActorUID;
    return pCamera;
  }

#ifndef NDEBUG
  RE_LOG(Warning, "Failed to create camera '%s'. Probably already exists.",
         name.c_str());
#endif
  return getCamera(name);
}

TResult core::MActors::destroyCamera(ACamera* pCamera) {
  const uint32_t UID = pCamera->getUID();

  if (m_actors.cameras.contains(UID)) {
    ACamera* pCamera = m_actors.cameras[UID].get();
    size_t index = 0;
    
    for (const auto& it : m_linearActors.pCameras) {
      if (it == pCamera) {
        break;
      }

      ++index;
    }

    // Remove the camera from the scene graph
    core::ref.unregisterCamera(pCamera);

    m_linearActors.pCameras.erase(m_linearActors.pCameras.begin() + index);
    m_actors.cameras.erase(UID);

    return RE_OK;
  }

#ifndef NDEBUG
  RE_LOG(Error, "Failed to destroy camera at %d.", pCamera);
#endif
  return RE_ERROR;
}

ACamera* core::MActors::getCamera(const std::string& name) {
  if (ACamera* pCamera = core::ref.getActor(name)->getAs<ACamera>()) {
    return pCamera;
  }

  return nullptr;
}

ALight* core::MActors::createLight(const std::string& name, RLightInfo* pInfo) {
  if (!core::ref.getActor(name)) {
    m_actors.lights[m_nextActorUID] = std::make_unique<ALight>(m_nextActorUID);
    ALight* pNewLight = m_actors.lights.at(m_nextActorUID).get();

    pNewLight->setName(name);

    if (pInfo) {
      pNewLight->setLightType(pInfo->type);
      pNewLight->setLightColor(pInfo->color);
      pNewLight->setLightIntensity(pInfo->intensity);
      pNewLight->setTranslation(pInfo->translation);
      pNewLight->setRotation(pInfo->direction);

      if (pInfo->isShadowCaster) {
        pNewLight->setAsShadowCaster(true);
        pNewLight->setOrthographic(2.0f, 2.0f, 0.001f, 1000.0f);

        // get free camera offset index into the dynamic buffer
        uint32_t index = 0;

        for (const auto& it : m_linearActors.pCameras) {
          if (it->getViewBufferIndex() != index) {
            break;
          }

          ++index;
        }

        pNewLight->setViewBufferIndex(index);
        m_linearActors.pCameras.emplace_back(pNewLight);
      }
    }

    core::ref.registerLight(pNewLight);
    m_linearActors.pLights.emplace_back(pNewLight);

#ifndef NDEBUG
    RE_LOG(Log, "Created light '%s'.", name.c_str());
#endif

    ++m_nextActorUID;
    return pNewLight;
  }

#ifndef NDEBUG
  RE_LOG(Warning, "Failed to create light '%s'. Probably already exists.",
    name.c_str());
#endif
  return getLight(name);
}

TResult core::MActors::destroyLight(ALight* pLight) {
  const uint32_t UID = pLight->getUID();

  uint32_t index = 0;
  if (m_actors.lights.contains(UID)) {
    for (const auto& it : m_linearActors.pLights) {
      if (it == pLight) {
        break;
      }

      ++index;
    }

    core::ref.unregisterLight(pLight);

    m_linearActors.pLights.erase(m_linearActors.pLights.begin() + index);

    m_actors.lights[UID].reset();
    m_actors.lights.erase(UID);

    return RE_OK;
  }

#ifndef NDEBUG
  RE_LOG(Error, "Failed to destroy light at %d.", pLight);
#endif

  return RE_ERROR;
}

ALight* core::MActors::getLight(const std::string& name) {
  if (ALight* pLight = core::ref.getActor(name)->getAs<ALight>()) {
    return pLight;
  }

  return nullptr;
}

bool core::MActors::setSunLight(const std::string& name) {
  return setSunLight(core::ref.getActor(name)->getAs<ALight>());
}

bool core::MActors::setSunLight(ALight* pLight) {
  if (pLight && pLight->isShadowCaster() && pLight->getLightType() == ELightType::Directional) {
    m_pSunLight = pLight;
    return true;
  }

  RE_LOG(Error, "Failed to set sun light. Either received a nullptr or it isn't a directional caster.");
  return false;
}

ALight* core::MActors::getSunLight() {
  return m_pSunLight;
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