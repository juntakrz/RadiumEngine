#include "pch.h"
#include "core/core.h"
#include "core/managers/actors.h"
#include "core/managers/input.h"
#include "core/world/model/primitive_plane.h"
#include "core/world/actors/camera.h"

core::MActors::MActors() {
  RE_LOG(Log, "Creating actors manager.");
}

ACamera* core::MActors::createCamera(const char* name,
                                     RCameraSettings* cameraSettings) {
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
  if (m_actors.cameras.find(name) != m_actors.cameras.end()) {
    m_actors.cameras.erase(name);
    return RE_OK;
  }

  RE_LOG(Error, "Failed to delete '%s' camera. Not found.", name);
  return RE_ERROR;
}

ACamera* core::MActors::getCamera(const char* name) {
  if (m_actors.cameras.find(name) != m_actors.cameras.end()) {
    return m_actors.cameras.at(name).get();
  }

  return nullptr;
}

TResult core::MActors::createMesh() {
  meshes.emplace_back(std::make_unique<WPrimitive_Plane>());
  meshes.back()->create();

  return RE_OK;
}

TResult core::MActors::destroyMesh(uint32_t index) {
  if (index < meshes.size()) {
    meshes[index]->destroy();
    meshes[index].reset();
    return RE_OK;
  }

  RE_LOG(Error,
         "failed to destroy mesh, probably incorrect index provided (%d)",
         index);
  return RE_ERROR;
}

void core::MActors::destroyAllMeshes() {
  RE_LOG(Log, "Clearing all mesh buffers and allocations.");

  for (auto& it : meshes) {
    it->destroy();
    it.reset();
  }

  meshes.clear();
}