#pragma once

#include "core/objects.h"
#include "core/world/primitivegen/primitivegen_include.h"

class WPrimitive {
 public:
  RBuffer vertexBuffer;
  RBuffer indexBuffer;
  uint32_t vertexCount = 0u;
  uint32_t indexCount = 0u;

  struct {
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
    bool isValid = false;
  } extent;

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

  virtual void create(int arg1 = 1, int arg2 = 1){};
  virtual void destroy();

  void setBoundingBoxExtent(const glm::vec3& min, const glm::vec3& max);

  // check return value first, has to be true for a valid bounding box extent
  const bool& getBoundingBoxExtent(glm::vec3& outMin, glm::vec3& outMax) const;
};