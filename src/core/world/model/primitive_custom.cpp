#include "core/world/model/primitive_custom.h"

WPrimitive_Custom::WPrimitive_Custom(const std::vector<RVertex>& vertexData,
                                     const std::vector<uint32_t>& indexData) {
  createBuffers(vertexData, indexData);
}

void WPrimitive_Custom::create(const std::vector<RVertex>& vertexData,
                               const std::vector<uint32_t>& indexData) {
  createBuffers(vertexData, indexData);
}
