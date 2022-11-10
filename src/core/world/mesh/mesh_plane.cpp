#include "pch.h"
#include "core/world/mesh/mesh.h"
#include "core/world/mesh/mesh_plane.h"
#include "core/world/primitives/plane.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"

void WMesh_Plane::create(int arg1, int arg2) {
  auto model = WPrimitive_Plane::create<RVertex>(arg1, arg2);
  vertices = model.vertices;
  indices = model.indices;

  //vertexBuffer.pData = vertices.data();

  allocateMemory();
}
