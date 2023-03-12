#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/world/model/primitive.h"

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

  // should create tangent space data - either now or later if generated primitive
  if (pCreateInfo->createTangentSpaceData) {
    createTangentSpaceData = true;

    if (!pCreateInfo->pVertexData || !pCreateInfo->pIndexData) {
      return;
    }

    generateTangentsAndBinormals(*pCreateInfo->pVertexData,
                                 *pCreateInfo->pIndexData);
  }
}
void WPrimitive::generatePlane(int32_t xDivisions, int32_t yDivisions,
                               std::vector<RVertex>& outVertices,
                               std::vector<uint32_t>& outIndices) noexcept {
  auto plane = WPrimitiveGen_Plane::create<RVertex>(xDivisions, yDivisions);

  plane.setNormals();
  plane.setColorForAllVertices(1.0f, 1.0f, 1.0f, 1.0f);

  outVertices = plane.vertices;
  outIndices = plane.indices;

  // store vertex and index count but offset must be provided to primitive externally
  vertexCount = static_cast<uint32_t>(outVertices.size());
  indexCount = static_cast<uint32_t>(outIndices.size());

  if (createTangentSpaceData) {
    generateTangentsAndBinormals(outVertices, outIndices);
  }
}
void WPrimitive::generateSphere(int32_t divisions, bool invertNormals,
                                std::vector<RVertex>& outVertices,
                                std::vector<uint32_t>& outIndices) noexcept {
  auto sphere = WPrimitiveGen_Sphere::create<RVertex>(divisions, invertNormals);

  sphere.setNormals();
  sphere.setColorForAllVertices(1.0f, 1.0f, 1.0f, 1.0f);

  outVertices = sphere.vertices;
  outIndices = sphere.indices;

  // store vertex and index count but offset must be provided to primitive externally
  vertexCount = static_cast<uint32_t>(outVertices.size());
  indexCount = static_cast<uint32_t>(outIndices.size());

  if (createTangentSpaceData) {
    generateTangentsAndBinormals(outVertices, outIndices);
  }
}
void WPrimitive::generateCube(int32_t divisions, bool invertNormals,
                              std::vector<RVertex>& outVertices,
                              std::vector<uint32_t>& outIndices) noexcept {
  auto cube = WPrimitiveGen_Cube::create<RVertex>(divisions, invertNormals);

  cube.setNormals();
  cube.setColorForAllVertices(1.0f, 1.0f, 1.0f, 1.0f);

  outVertices = cube.vertices;
  outIndices = cube.indices;

  // store vertex and index count but offset must be provided to primitive
  // externally
  vertexCount = static_cast<uint32_t>(outVertices.size());
  indexCount = static_cast<uint32_t>(outIndices.size());

  if (createTangentSpaceData) {
    generateTangentsAndBinormals(outVertices, outIndices);
  }
}
void WPrimitive::generateBoundingBox(
    std::vector<RVertex>& outVertices,
    std::vector<uint32_t>& outIndices) noexcept {
  // not fully implemented yet, likely needs WModel to be expanded
};

void WPrimitive::setBoundingBoxExtent(const glm::vec3& min,
                                      const glm::vec3& max) {
  extent.min = min;
  extent.max = max;
  extent.isValid = true;
}

bool WPrimitive::getBoundingBoxExtent(glm::vec3& outMin, glm::vec3& outMax) const {
  if (extent.isValid) {
    outMin = extent.min;
    outMax = extent.max;
  }
  return extent.isValid;
}

void WPrimitive::generateTangentsAndBinormals(
    std::vector<RVertex>& vertexData,
    const std::vector<uint32_t>& inIndexData) {
  if (inIndexData.size() % 3 != 0 || inIndexData.size() < 3) {
    return;
  }

  for (uint32_t i = 0; i < inIndexData.size(); i += 3) {
    // load vertices
    auto& vertex0 = vertexData[inIndexData[i]];
    auto& vertex1 = vertexData[inIndexData[i + 1]];
    auto& vertex2 = vertexData[inIndexData[i + 2]];

    // geometry vectors
    const glm::vec3 vec0 = vertex0.pos;
    const glm::vec3 vec1 = vertex1.pos;
    const glm::vec3 vec2 = vertex2.pos;

    const glm::vec3 vecSide_1_0 = vec1 - vec0;
    const glm::vec3 vecSide_2_0 = vec2 - vec0;

    // texture vectors
    const glm::vec2 xmU = {vertex1.tex0.x - vertex0.tex0.x,
                                   vertex2.tex0.x - vertex0.tex0.x};
    const glm::vec2 xmV = {vertex1.tex0.y - vertex0.tex0.y,
                                   vertex2.tex0.y - vertex0.tex0.y};

    // denominator of the tangent/binormal equation: 1.0 / cross(vecX, vecY)
    const float den = 1.0f / (xmU.x * xmV.y - xmU.y * xmV.x);

    // calculate tangent
    glm::vec3 tangent;

    tangent.x = (xmV.y * vecSide_1_0.x - xmV.x * vecSide_2_0.x) * den;
    tangent.y = (xmV.y * vecSide_1_0.y - xmV.x * vecSide_2_0.y) * den;
    tangent.z = (xmV.y * vecSide_1_0.z - xmV.x * vecSide_2_0.z) * den;

    // calculate binormal
    glm::vec3 binormal;

    binormal.x = (xmU.x * vecSide_2_0.x - xmU.y * vecSide_1_0.x) * den;
    binormal.y = (xmU.x * vecSide_2_0.y - xmU.y * vecSide_1_0.y) * den;
    binormal.z = (xmU.x * vecSide_2_0.z - xmU.y * vecSide_1_0.z) * den;

    // normalize binormal and tangent
    tangent = glm::normalize(tangent);
    binormal = glm::normalize(binormal);

    // store new data to vertex
    vertex0.tangent = tangent;
    vertex0.binormal = -binormal;

    vertex1.tangent = tangent;
    vertex1.binormal = -binormal;

    vertex2.tangent = tangent;
    vertex2.binormal = -binormal;

    // setNormals();
  }
}

void WPrimitive::setNormalsFromVertices(std::vector<RVertex>& vertexData) {
  for (auto& vertex : vertexData) {
    vertex.normal = glm::normalize(vertex.pos);
  }
}