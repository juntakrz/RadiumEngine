#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/model/primitive.h"

WPrimitive::WPrimitive(RPrimitiveInfo* pCreateInfo) {
  // will check for create info data validity only if vertex data was provided
  if ((pCreateInfo->vertexCount < 3 || pCreateInfo->indexCount < 3) &&
      pCreateInfo->pVertexData) {
    RE_LOG(Error, "Invalid primitive creation data provided.");
    return;
  }

  vertexOffset = pCreateInfo->vertexOffset;
  vertexCount = pCreateInfo->vertexCount;
  indexOffset = pCreateInfo->indexOffset;
  indexCount = pCreateInfo->indexCount;

  pOwnerNode = pCreateInfo->pOwnerNode;
}
void WPrimitive::generatePlane(int32_t xDivisions, int32_t yDivisions,
                               std::vector<RVertex>& outVertices,
                               std::vector<uint32_t>& outIndices) noexcept {
  auto plane = WPrimitiveGen_Plane::create<RVertex>(xDivisions, yDivisions);

  plane.setNormals(false, false);
  plane.setColorForAllVertices(1.0f, 1.0f, 1.0f, 1.0f);

  outVertices = plane.vertices;
  outIndices = plane.indices;

  // store vertex and index count but offset must be provided to primitive externally
  vertexCount = static_cast<uint32_t>(outVertices.size());
  indexCount = static_cast<uint32_t>(outIndices.size());
}
void WPrimitive::generateSphere(int32_t divisions, bool invertNormals,
                                std::vector<RVertex>& outVertices,
                                std::vector<uint32_t>& outIndices) noexcept {
  auto sphere = WPrimitiveGen_Sphere::create<RVertex>(divisions, invertNormals);

  sphere.setNormals(invertNormals, false);
  sphere.setColorForAllVertices(1.0f, 1.0f, 1.0f, 1.0f);

  outVertices = sphere.vertices;
  outIndices = sphere.indices;

  // store vertex and index count but offset must be provided to primitive externally
  vertexCount = static_cast<uint32_t>(outVertices.size());
  indexCount = static_cast<uint32_t>(outIndices.size());
}
void WPrimitive::generateCube(int32_t divisions, bool invertNormals,
                              std::vector<RVertex>& outVertices,
                              std::vector<uint32_t>& outIndices) noexcept {
  auto cube = WPrimitiveGen_Cube::create<RVertex>(divisions, false);

  cube.setNormals(invertNormals, invertNormals);
  cube.setColorForAllVertices(1.0f, 1.0f, 1.0f, 1.0f);

  outVertices = cube.vertices;
  outIndices = cube.indices;

  // store vertex and index count but offset must be provided to primitive
  // externally
  vertexCount = static_cast<uint32_t>(outVertices.size());
  indexCount = static_cast<uint32_t>(outIndices.size());
}

void WPrimitive::setBoundingBoxExtent(const glm::vec3& min,
                                      const glm::vec3& max) {
  extent.min = min;
  extent.max = max;

  float maxValue = std::max(max.x, std::max(max.y, max.z));
  float minValue = std::min(min.x, std::min(min.y, min.z));
  extent.magnitude = maxValue - minValue;

  // Generate 8 corners out of bounding box min / max extent
  extent.boxCorners[0] = {min.x, max.y, max.z, 1.0f};      // 0                0 ______ 1
  extent.boxCorners[1] = {max.x, max.y, max.z, 1.0f};      // 1              / |      / |
  extent.boxCorners[2] = {min.x, max.y, min.z, 1.0f};      // 2            2 __|____ 3  |
  extent.boxCorners[3] = {max.x, max.y, min.z, 1.0f};      // 3            |   |     |  |
  extent.boxCorners[4] = {min.x, min.y, max.z, 1.0f};      // 4            |   4 ___ |_ 5
  extent.boxCorners[5] = {max.x, min.y, max.z, 1.0f};      // 5            | /       | /
  extent.boxCorners[6] = {min.x, min.y, min.z, 1.0f};      // 6            6 _______ 7
  extent.boxCorners[7] = {max.x, min.y, min.z, 1.0f};      // 7

  extent.isValid = true;
}

bool WPrimitive::getBoundingBoxExtent(glm::vec3& outMin, glm::vec3& outMax) const {
  if (extent.isValid) {
    outMin = extent.min;
    outMax = extent.max;
  }
  return extent.isValid;
}

void WPrimitive::setNormalsFromVertices(std::vector<RVertex>& vertexData) {
  for (auto& vertex : vertexData) {
    vertex.normal = glm::normalize(vertex.pos);
  }
}