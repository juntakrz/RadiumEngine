#pragma once

#include "mesh.h"

class WMesh_Plane : public WMesh {
 public:
  virtual void create(int arg1 = 1, int arg2 = 1) override;
};