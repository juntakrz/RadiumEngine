#include "pch.h"
#include "core/world/model/primitive.h"
#include "core/world/model/primitive_plane.h"
#include "core/world/primitivegen/plane.h"
#include "core/core.h"
#include "core/managers/renderer.h"

void WPrimitive_Plane::create(int arg1, int arg2) {
  auto model = WPrimitiveGen_Plane::create<RVertex>(arg1, arg2);

  createBuffers(model.vertices, model.indices);
}
