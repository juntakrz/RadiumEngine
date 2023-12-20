#include "pch.h"
#include "core/core.h"
#include "core/managers/input.h"
#include "core/world/actors/camera.h"
#include "core/managers/actors.h"

core::MActors::MActors() {
  RE_LOG(Log, "Creating actors manager.");
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

    RE_LOG(Log, "Created camera '%s'.", name);
    return pCamera;
  }

  RE_LOG(Error,
         "Failed to create camera '%s'. Probably already exists. Attempting to "
         "find it.",
         name);
  return getCamera(name);
}

TResult core::MActors::destroyCamera(const char* name) {
  if (m_actors.cameras.contains(name)) {
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

APawn* core::MActors::createPawn(const char* name) {
  if (!m_actors.pawns.try_emplace(name).second) {
    RE_LOG(Error, "Failed to created pawn '%s'. It probably already exists.",
           name);
    return nullptr;
  };

  m_actors.pawns.at(name) = std::make_unique<APawn>();

  // should probably add a reference to MRef here?

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

  // should probably add a reference to MRef here?

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