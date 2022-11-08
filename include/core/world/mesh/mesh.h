#pragma once

#include "pch.h"
#include "core/core.h"
#include "core/objects.h"
#include "core/managers/mgraphics.h"

class WMesh {
 protected:
  void setMemory();

 public:
  RBuffer vertexBuffer;

  std::vector<RVertex> vertices;
  std::vector<uint32_t> indices;

  virtual void create(int arg1, int arg2) = 0;
  virtual void destroy();
};