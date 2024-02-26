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
    pLightingBuffer->lightLocations[0] = glm::vec4(m_pSunLight->getLocation(), 1.0f);
    pLightingBuffer->lightColors[0] = glm::vec4(m_pSunLight->getLightColor(), m_pSunLight->getLightIntensity());
    pLightingBuffer->lightViews[0] = m_pSunLight->getView();
    pLightingBuffer->lightOrthoMatrix = m_pSunLight->getProjection();
  }

  // Currently all other lights are expected to be point lights
  for (const auto& pLight : m_linearActors.pLights) {
    if (pLight->isVisible() && pLight != m_pSunLight) {
      pLightingBuffer->lightLocations[lightCount] = glm::vec4(pLight->getLocation(), 1.0f);
      pLightingBuffer->lightColors[lightCount] = glm::vec4(pLight->getLightColor(), pLight->getLightIntensity());

      ++lightCount;
    }

    // stop if max visible lights limit was reached
    if (lightCount == RE_MAXLIGHTS) break;
  }

  pLightingBuffer->lightCount = lightCount;
}

ACamera* core::MActors::createCamera(const char* name,
                                     RCameraInfo* cameraSettings) {
  if (m_actors.cameras.try_emplace(name).second) {
    m_actors.cameras.at(name) = std::make_unique<ACamera>();
    ACamera* pCamera = m_actors.cameras.at(name).get();
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

    core::ref.registerActor(name, pCamera);

    RE_LOG(Log, "Created camera '%s'.", name);
    return pCamera;
  }

#ifndef NDEBUG
  RE_LOG(Warning, "Failed to create camera '%s'. Probably already exists.",
         name);
#endif
  return getCamera(name);
}

TResult core::MActors::destroyCamera(const char* name) {
  if (m_actors.cameras.contains(name)) {
    ACamera* pCamera = m_actors.cameras.at(name).get();
    size_t index = 0;
    
    for (const auto& it : m_linearActors.pCameras) {
      if (it == pCamera) {
        break;
      }

      ++index;
    }

    m_linearActors.pCameras.erase(m_linearActors.pCameras.begin() + index);
    m_actors.cameras.erase(name);
    return RE_OK;
  }

  RE_LOG(Error, "Failed to delete '%s' camera. Not found.", name);
  return RE_ERROR;
}

ACamera* core::MActors::getCamera(const char* name) {
  if (m_actors.cameras.contains(name)) {
    return m_actors.cameras.at(name).get();
  }

  return nullptr;
}

ALight* core::MActors::createLight(const char* name, RLightInfo* pInfo) {
  if (m_actors.lights.contains(name)) {
    return m_actors.lights.at(name).get();
  }

  m_actors.lights[name] = std::make_unique<ALight>();
  ALight* pNewLight = m_actors.lights.at(name).get();

  pNewLight->setName(name);

  if (pInfo) {
    pNewLight->setLightType(pInfo->type);
    pNewLight->setLightColor(pInfo->color);
    pNewLight->setLightIntensity(pInfo->intensity);
    pNewLight->setLocation(pInfo->translation);
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

  // should probably add a reference to MRef here?

  RE_LOG(Log, "Created light '%s'.", name);

  m_linearActors.pLights.emplace_back(m_actors.lights.at(name).get());
  return m_actors.lights.at(name).get();
}

TResult core::MActors::destroyLight(const char* name) {
  if (m_actors.lights.contains(name)) {
    m_actors.lights.at(name).reset();
    m_actors.lights.erase(name);

#ifndef NDEBUG
    RE_LOG(Log, "Destroyed light '%s'.", name);
#endif

    return RE_OK;
  }

#ifndef NDEBUG
  RE_LOG(Warning, "Couldn't destroy light '%s'. Was not found.", name);
#endif

  return RE_WARNING;
}

ALight* core::MActors::getLight(const char* name) {
  if (m_actors.lights.contains(name)) {
    return m_actors.lights.at(name).get();
  }

  RE_LOG(Error, "Failed to get light '%s' from actor list. It does not exist.",
         name);
  return nullptr;
}

bool core::MActors::setSunLight(const char* name) {
  if (m_actors.lights.contains(name)) {
    ALight* pLight = m_actors.lights.at(name).get();

    if (pLight->isShadowCaster() && pLight->getLightType() == ELightType::Directional) {
      m_pSunLight = pLight;
      return true;
    }
  }

  RE_LOG(Error, "Failed to set '%s' as sun light. It either does not exist or isn't a directional caster.", name);
  return false;
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

APawn* core::MActors::createPawn(const char* name) {
  if (!m_actors.pawns.try_emplace(name).second) {
    RE_LOG(Error, "Failed to created pawn '%s'. It probably already exists.",
           name);
    return nullptr;
  };

  m_actors.pawns.at(name) = std::make_unique<APawn>();

  // should probably add a reference to MRef here?

  m_linearActors.pPawns.emplace_back(m_actors.pawns.at(name).get());
  return m_actors.pawns.at(name).get();
}

TResult core::MActors::destroyPawn(const char* name) {
  if (m_actors.pawns.contains(name)){
    if (m_actors.pawns.at(name)->getRendererBindingIndex() > -1) {
      RE_LOG(Error,
             "Failed to destroy pawn \"%s\". It is still bound to rendering "
             "pipeline.");

      return RE_ERROR;
    }

    m_actors.pawns.erase(name);
    return RE_OK;
  }

  RE_LOG(Error, "Failed to destroy pawn '%s', doesn't exist.", name);
  return RE_ERROR;
}

APawn* core::MActors::getPawn(const char* name) {
  if (m_actors.pawns.contains(name)) {
    return m_actors.pawns.at(name).get();
  }

  RE_LOG(Error, "Failed to get pawn '%s'.", name);
  return nullptr;
}

AStatic* core::MActors::createStatic(const char* name) {
  if (!m_actors.statics.try_emplace(name).second) {
    RE_LOG(Error, "Failed to created static '%s'. It probably already exists.",
           name);
    return nullptr;
  };

  m_actors.statics.at(name) = std::make_unique<AStatic>();
  m_actors.statics.at(name)->setName(name);

  // should probably add a reference to MRef here?

  m_linearActors.pStatics.emplace_back(m_actors.statics.at(name).get());
  return m_actors.statics.at(name).get();
}

TResult core::MActors::destroyStatic(const char* name) {
  if (m_actors.statics.contains(name)) {
    if (m_actors.statics.at(name)->getRendererBindingIndex() > -1) {
      RE_LOG(Error,
             "Failed to destroy static \"%s\". It is still bound to rendering "
             "pipeline.");

      return RE_ERROR;
    }

    m_actors.statics.erase(name);
    return RE_OK;
  }

  RE_LOG(Error, "Failed to destroy static '%s', doesn't exist.", name);
  return RE_ERROR;
}

AStatic* core::MActors::getStatic(const char* name) {
  if (m_actors.statics.contains(name)) {
    return m_actors.statics.at(name).get();
  }

  RE_LOG(Error, "Failed to get static '%s'.", name);
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