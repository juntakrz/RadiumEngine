#include "pch.h"
#include "core/world/model/primitive_custom.h"

WPrimitive_Custom::WPrimitive_Custom(std::vector<RVertex>& vertexData,
                                     const std::vector<uint32_t>& indexData) {
  generateTangentsAndBinormals(vertexData, indexData);
  //setNormalsFromVertices(vertexData);
  createBuffers(vertexData, indexData);
}

void WPrimitive_Custom::create(std::vector<RVertex>& vertexData,
                               const std::vector<uint32_t>& indexData) {
  generateTangentsAndBinormals(vertexData, indexData);
  //setNormalsFromVertices(vertexData);
  createBuffers(vertexData, indexData);
}