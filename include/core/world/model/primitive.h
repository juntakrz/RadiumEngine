#pragma once

#include "core/objects.h"
#include "core/managers/materials.h"
#include "core/world/primitivegen/primitivegen_include.h"

class WPrimitive {
  using RMaterial = core::MMaterials::RMaterial;

 public:
  uint32_t vertexOffset = 0u;    // initial vertex location in owning model
  uint32_t indexOffset = 0u;     // initial index location in owning model
  uint32_t vertexCount = 0u;
  uint32_t indexCount = 0u;
  bool createTangentSpaceData = false;

  RMaterial* pMaterial = nullptr;

  struct {
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
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

  void generateBoundingBox(std::vector<RVertex>& outVertices,
                           std::vector<uint32_t>& outIndices) noexcept;


  void setBoundingBoxExtent(const glm::vec3& min, const glm::vec3& max);

  // check return value first, has to be true for a valid bounding box extent
  bool getBoundingBoxExtent(glm::vec3& outMin, glm::vec3& outMax) const;

  void generateTangentsAndBinormals(std::vector<RVertex>& vertexData,
                                    const std::vector<uint32_t>& inIndexData);
  void setNormalsFromVertices(std::vector<RVertex>& vertexData);
};