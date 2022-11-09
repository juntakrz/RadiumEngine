#pragma once

#include "common.h"
#include "core/world/mesh/mesh.h"

class MModel {
  MModel();

 public:
  static MModel& get() {
    static MModel _sInstance;
    return _sInstance;
  }

  MModel(const MModel&) = delete;
  MModel& operator=(const MModel&) = delete;

  TResult createMesh();
  TResult destroyMesh(uint32_t index);
  void destroyAllMeshes();

  std::vector<std::unique_ptr<WMesh>> meshes;
};