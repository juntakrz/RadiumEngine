#pragma once

#include "pch.h"
#include "core/objects.h"

class WMesh {
 public:
  std::vector<RVertex> vertices;
  std::vector<uint32_t> indices;

  virtual void create(int arg1, int arg2) = 0;
};