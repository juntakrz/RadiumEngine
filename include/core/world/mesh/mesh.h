#pragma once

#include "core/objects.h"

class WMesh {
 protected:
  // allocate memory for vertex and index buffers
  virtual void allocateMemory();

 public:
  WMesh();
  virtual ~WMesh(){};

  virtual void create(int arg1 = 1, int arg2 = 1) = 0;
  virtual void destroy();

 public:
  RBuffer vertexBuffer;
  RBuffer stagingVertexBuffer;

  RBuffer indexBuffer;
  RBuffer stagingIndexBuffer;

  std::vector<RVertex> vertices;
  std::vector<uint32_t> indices;
};