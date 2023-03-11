#include "pch.h"
#include "core/world/primitivegen/sphere.h"
#include "core/world/model/primitive_sphere.h"

void WPrimitive_Sphere::create(int arg0, int arg1) {
  auto model = WPrimitiveGen_Sphere::create<RVertex>(arg0, arg1);
  model.setNormals();
  model.setColorForAllVertices(1.0f, 1.0f, 1.0f, 1.0f);

  //
}