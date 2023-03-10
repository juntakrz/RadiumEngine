#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/world/model/primitive.h"

WPrimitive::WPrimitive() {
  vertexBuffer.buffer = VK_NULL_HANDLE;
  vertexBuffer.allocation = VK_NULL_HANDLE;

  indexBuffer.buffer = VK_NULL_HANDLE;
  indexBuffer.allocation = VK_NULL_HANDLE;
};

void WPrimitive::createVertexBuffer(const std::vector<RVertex>& vertexData) {
  VkDeviceSize size = vertexData.size() * sizeof(RVertex);
  core::renderer.createBuffer(EBufferMode::DGPU_VERTEX, size, vertexBuffer,
                              (void*)vertexData.data());
  vertexCount = static_cast<uint32_t>(vertexData.size());
}

void WPrimitive::createIndexBuffer(const std::vector<uint32_t>& indexData) {
  VkDeviceSize size = indexData.size() * sizeof(indexData[0]);
  core::renderer.createBuffer(EBufferMode::DGPU_INDEX, size, indexBuffer,
                              (void*)indexData.data());
  indexCount = static_cast<uint32_t>(indexData.size());
}

void WPrimitive::createBuffers(const std::vector<RVertex>& vertexData,
                          const std::vector<uint32_t>& indexData) {
  createVertexBuffer(vertexData);
  createIndexBuffer(indexData);
}

void WPrimitive::destroy() {
  vmaDestroyBuffer(core::renderer.memAlloc, vertexBuffer.buffer,
                   vertexBuffer.allocation);
  vmaDestroyBuffer(core::renderer.memAlloc, indexBuffer.buffer,
                   indexBuffer.allocation);
}

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