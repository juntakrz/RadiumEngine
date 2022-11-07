#pragma once

#include "triangleindex.h"

class WPlane {
 public:
  template <class V>
  static WTriangleIndex<V> create(const uint16_t divX, const uint16_t divY) {
    ASSERT(divX > 0);
    ASSERT(divY > 0);

    constexpr float coord = 2.0f;
    float stepX = coord / (float)divX;
    float stepY = coord / (float)divY;
    uint16_t divisions = divX * divY;

    std::vector<V> vertices;

    for (uint16_t i = 0; i <= divX; i++) {
      for (uint16_t j = 0; j <= divY; j++) {
        vertices.emplace_back();
        vertices.back().pos = {stepX * i - 1, 1 - (stepY * j), 0.0f};
        vertices.back().tex = {stepX * i / 2, stepY * j / 2};
      }
    }

    std::vector<uint32_t> indices;
    uint16_t lastStep = divisions + divX - 1;

    for (uint16_t k = 0, skipStep = 1; k < lastStep; k++, skipStep++) {
      // skip invalid polygons (happens after every divX value)
      if (skipStep <= divY) {
        indices.emplace_back(k);
        indices.emplace_back(k + divY + 1);
        indices.emplace_back(k + 1);

        indices.emplace_back(k + 1);
        indices.emplace_back(k + divY + 1);
        indices.emplace_back(k + divY + 2);
      } else {
        skipStep = 0;
      }
    }

    return {std::move(vertices), std::move(indices)};
  }

  template <class V>
  static WTriangleIndex<V> create(uint16_t divisions) {
    return Create<V>(divisions, divisions);
  }

  template <class V>
  static WTriangleIndex<V> create() {
    return Create<V>(1);
  }
};