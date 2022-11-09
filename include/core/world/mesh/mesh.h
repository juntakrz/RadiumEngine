#pragma once

#include "core/objects.h"

class WMesh {
 protected:
  void setMemory();

 public:
  WMesh(){};
  virtual ~WMesh(){};

  virtual void create(int arg1 = 1, int arg2 = 1) = 0;
  virtual void destroy();

 public:
  RBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;

  std::vector<RVertex> vertices;
  std::vector<uint32_t> indices;
};