#include "pch.h"
#include "core/world/model/mesh.h"
#include "core/world/model/mesh_plane.h"
#include "core/world/primitives/plane.h"
#include "core/core.h"
#include "core/managers/renderer.h"

void WMesh_Plane::create(int arg1, int arg2) {
  auto model = WPrimitive_Plane::create<RVertex>(arg1, arg2);

  createBuffers(model.vertices, model.indices);
}
