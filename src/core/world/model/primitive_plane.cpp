#include "pch.h"
#include "core/world/primitivegen/plane.h"
#include "core/world/model/primitive_plane.h"

void WPrimitive_Plane::create(int arg0, int arg1) {
  auto model = WPrimitiveGen_Plane::create<RVertex>(arg0, arg1);

  //
}
