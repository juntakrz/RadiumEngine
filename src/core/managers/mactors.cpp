#include "pch.h"
#include "core/core.h"
#include "core/managers/mactors.h"
#include "core/managers/minput.h"
#include "core/world/mesh/mesh_plane.h"
#include "core/world/actors/acamera.h"

core::MActors::MActors() {
  RE_LOG(Log, "Creating mesh and model manager.");
  bindDefaultMethods();
}

void core::MActors::bindDefaultMethods() {
  RE_LOG(Log, "Binding default actor methods.");

  core::input.bindFunction(GETKEY("moveForward"), GLFW_PRESS, this,
                           &MActors::translateForward);
  core::input.bindFunction(GETKEY("moveBack"), GLFW_PRESS, this,
                           &MActors::translateBack);
  core::input.bindFunction(GETKEY("moveLeft"), GLFW_PRESS, this,
                           &MActors::translateLeft);
  core::input.bindFunction(GETKEY("moveRight"), GLFW_PRESS, this,
                           &MActors::translateRight);
  core::input.bindFunction(GETKEY("moveUp"), GLFW_PRESS, this,
                           &MActors::translateUp);
  core::input.bindFunction(GETKEY("moveDown"), GLFW_PRESS, this,
                           &MActors::translateDown);
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
  meshes.emplace_back(std::make_unique<WMesh_Plane>());
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

void core::MActors::controlActor(ABase* pActor) {
  for (const auto it : m_inputActors) {
    if (it == pActor) {
      RE_LOG(Warning,
             "Actor at %d seems to be already receiving input, won't assign.",
             pActor);
      return;
    }
  }

  m_inputActors.emplace_back(pActor);
}

void core::MActors::controlCamera(const char* name) {}

void core::MActors::freeActor(ABase* pActor) {
  for (const auto it : m_inputActors) {
    if (it == pActor) {
      //m_inputActors.erase(it);
      return;
    }
  }

  RE_LOG(Warning,
         "Actor at %d is not receiving input, won't unassign.",
         pActor);
}

void core::MActors::translateForward() {
  RE_LOG(Log, "%s: Moving forward.", __FUNCTION__);
}

void core::MActors::translateBack() {
  RE_LOG(Log, "%s: Moving back.", __FUNCTION__);
}

void core::MActors::translateLeft() {
  RE_LOG(Log, "%s: Moving left.", __FUNCTION__);
}

void core::MActors::translateRight() {
  RE_LOG(Log, "%s: Moving right.", __FUNCTION__);
}

void core::MActors::translateUp() {
  RE_LOG(Log, "%s: Moving up.", __FUNCTION__);
}

void core::MActors::translateDown() {
  RE_LOG(Log, "%s: Moving down.", __FUNCTION__);
}
