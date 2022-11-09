#include "pch.h"
#include "core/managers/mmodel.h"
#include "core/world/mesh/mesh_plane.h"

MModel::MModel() { RE_LOG(Log, "Creating mesh and model manager."); }

TResult MModel::createMesh() {
  meshes.emplace_back(std::make_unique<WMesh_Plane>());
  meshes.back()->create();

  return RE_OK;
}