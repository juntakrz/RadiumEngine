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

 protected:
  const VkMemoryPropertyFlags meshMemFlags =
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  const VkBufferUsageFlags vertexBufferFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

 public:
  RBuffer vertexBuffer;

  std::vector<RVertex> vertices;
  std::vector<uint32_t> indices;
};