#include "pch.h"
#include "core/managers/mmodel.h"
#include "core/world/mesh/mesh_plane.h"

MModel::MModel() { RE_LOG(Log, "Creating mesh and model manager."); }

TResult MModel::createMesh() {
  meshes.emplace_back(std::make_unique<WMesh_Plane>());
  meshes.back()->create();

  return RE_OK;
}

TResult MModel::destroyMesh(uint32_t index) {
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

void MModel::destroyAllMeshes() {
  for (auto& it : meshes) {
    it->destroy();
    it.reset();
  }

  meshes.clear();
}
