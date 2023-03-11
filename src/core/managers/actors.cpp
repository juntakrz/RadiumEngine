#include "pch.h"
#include "core/core.h"
#include "core/managers/input.h"
#include "core/world/model/primitive_plane.h"
#include "core/world/actors/camera.h"
#include "core/managers/actors.h"

core::MActors::MActors() {
  RE_LOG(Log, "Creating actors manager.");
}

ACamera* core::MActors::createCamera(const char* name,
                                     RCameraInfo* cameraSettings) {
  if (m_actors.cameras.try_emplace(name).second) {
    m_actors.cameras.at(name) = std::make_unique<ACamera>();

    if (cameraSettings) {
      m_actors.cameras.at(name)->setPerspective(
          cameraSettings->FOV, cameraSettings->aspectRatio,
          cameraSettings->nearZ, cameraSettings->farZ);
    } else {
      m_actors.cameras.at(name)->setPerspective(
          glm::radians(75.0f), config::getAspectRatio(),
          RE_NEARZ, config::viewDistance);
    }

    RE_LOG(Log, "Created camera '%s'.", name);
    return m_actors.cameras.at(name).get();
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

TResult core::MActors::createPawn(const char* name) {
  if (!m_actors.pawns.try_emplace(name).second) {
    RE_LOG(Error, "Failed to created pawn '%s'. It probably already exists.",
           name);
    return RE_ERROR;
  };

  m_actors.pawns.at(name) = std::make_unique<APawn>();

  // should probably add a reference to MRef here?

  return RE_OK;
}

TResult core::MActors::destroyPawn(const char* name) {
  if (m_actors.pawns.contains(name)){
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

void core::MActors::destroyAllPawns() {
  RE_LOG(Log, "Clearing all pawn buffers and allocations.");

  for (auto& it : m_actors.pawns) {
    it.second.reset();
  }

  m_actors.pawns.clear();
}