#pragma once

#include "core/objects.h"
#include "core/world/primitivegen/primitivegen_include.h"

class WPrimitive {
 private:
  void createVertexBuffer(const std::vector<RVertex>& vertexData);
  void createIndexBuffer(const std::vector<uint32_t>& indexData);

 protected:
  // allocate memory and create vertex and index buffers
  virtual void createBuffers(const std::vector<RVertex>& vertexData,
                             const std::vector<uint32_t>& indexData);

 public:
  WPrimitive();
  virtual ~WPrimitive(){};

  virtual void create(int arg1 = 1, int arg2 = 1) = 0;
  virtual void destroy();

 public:
  RBuffer vertexBuffer;
  RBuffer indexBuffer;
  uint32_t vertexCount = 0u;
  uint32_t indexCount = 0u;
};