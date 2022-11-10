#pragma once

#include "core/objects.h"

class WMesh {
 private:
  void createVertexBuffer();
  void createIndexBuffer();

 protected:
  virtual void allocateMemory();        // allocate memory for vertex and index buffers

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