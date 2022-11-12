#include "pch.h"
#include "core/managers/mactors.h"
#include "core/world/mesh/mesh_plane.h"

core::MActors::MActors() { RE_LOG(Log, "Creating mesh and model manager."); }

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
