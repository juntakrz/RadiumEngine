#pragma once

#include "core/objects.h"
#include "core/managers/resources.h"
#include "core/model/primitivegen_include.h"

class WPrimitive {
 public:
  struct WPrimitiveInstanceInfo {
     uint32_t firstVisibleInstance = -1;
     uint32_t visibleInstanceCount = 0;
  };

  uint32_t vertexOffset = 0u;    // initial vertex location in owning model
  uint32_t indexOffset = 0u;     // initial index location in owning model
  uint32_t vertexCount = 0u;
  uint32_t indexCount = 0u;

  RMaterial* pInitialMaterial = nullptr;
  void* pOwnerNode = nullptr;
  
  std::vector<WPrimitiveInstanceInfo> instanceInfo;
  std::vector<WPrimitiveInstanceData> instanceData;

  struct {
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
    glm::vec4 boxCorners[8];
    float magnitude = 0.0f;
    bool isValid = false;
  } extent;

 public:
  WPrimitive() = delete;
  WPrimitive(RPrimitiveInfo* pCreateInfo);
  virtual ~WPrimitive(){};

  void generatePlane(int32_t xDivisions, int32_t yDivisions,
                     std::vector<RVertex>& outVertices,
                     std::vector<uint32_t>& outIndices) noexcept;

  void generateSphere(int32_t divisions, bool invertNormals,
                      std::vector<RVertex>& outVertices,
                      std::vector<uint32_t>& outIndices) noexcept;

  // 'divisions' variable is not implemented yet
  void generateCube(int32_t divisions, bool invertNormals,
                    std::vector<RVertex>& outVertices,
                    std::vector<uint32_t>& outIndices) noexcept;


  void setBoundingBoxExtent(const glm::vec3& min, const glm::vec3& max);

  // check return value first, has to be true for a valid bounding box extent
  bool getBoundingBoxExtent(glm::vec3& outMin, glm::vec3& outMax) const;

  void setNormalsFromVertices(std::vector<RVertex>& vertexData);
};