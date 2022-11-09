#pragma once

#include "core/objects.h"

class WMesh {
 protected:
  WMesh(){};
  virtual ~WMesh(){};

  void setMemory();

 public:
  RBuffer vertexBuffer;

  std::vector<RVertex> vertices;
  std::vector<uint32_t> indices;

  virtual void create(int arg1, int arg2) = 0;
  virtual void destroy();
};