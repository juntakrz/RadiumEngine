#pragma once

#include "pch.h"

template <class V>
class WPrimitiveData {
 public:
  std::vector<V> vertices;
  std::vector<uint32_t> indices;

  WPrimitiveData() = default;
  WPrimitiveData(std::vector<V> inVerts, std::vector<uint32_t> inIndices)
      : vertices(std::move(inVerts)), indices(std::move(inIndices)) {
    ASSERT(vertices.size() > 2);
    ASSERT(indices.size() % 3 == 0);
  }

  void setNormals(bool flipNormals, bool flipVertices) noexcept {
    for (auto& it : vertices) {
      it.pos = flipVertices ? -it.pos : it.pos;
      it.normal = flipNormals ? -it.pos : it.pos;
      it.normal = glm::normalize(it.normal);
    }
  }

  void setColorForAllVertices(float r, float g, float b, float a) {
    for (auto& it : vertices) {
      it.color = {r,g,b,a};
    }
  }
};