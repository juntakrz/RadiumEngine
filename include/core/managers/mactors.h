#pragma once

#include "common.h"
#include "core/world/mesh/mesh.h"

namespace core {

  class MActors {
    MActors();

  public:
    static MActors& get() {
      static MActors _sInstance;
      return _sInstance;
    }

    MActors(const MActors&) = delete;
    MActors& operator=(const MActors&) = delete;

    TResult createMesh();
    TResult destroyMesh(uint32_t index);
    void destroyAllMeshes();

    std::vector<std::unique_ptr<WMesh>> meshes;
  };
}