#pragma once

#include "triangleindex.h"
#include "util/util.h"

class WPrimitive_Plane {
 public:
  template <class V>
  static WTriangleIndex<V> create(const int& divX, const int& divY) {
    ASSERT(divX > 0);
    ASSERT(divY > 0);

    constexpr float coord = 2.0f;
    float stepX = coord / (float)divX;
    float stepY = coord / (float)divY;
    int divisions = divX * divY;

    std::vector<V> vertices;

    for (int i = 0; i <= divX; i++) {
      for (int j = 0; j <= divY; j++) {
        vertices.emplace_back();
        vertices.back().pos = {stepX * i - 1, 1 - (stepY * j), 0.0f};
        //vertices.back().tex = {stepX * i / 2, stepY * j / 2};
        vertices.back().color = {random(0.0f, 1.0f), random(0.0f, 1.0f),
                                 random(0.0f, 1.0f)};
      }
    }

    std::vector<uint32_t> indices;
    uint16_t lastStep = divisions + divX - 1;

    for (int k = 0, skipStep = 1; k < lastStep; k++, skipStep++) {
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
  static WTriangleIndex<V> create(int divisions) {
    return create<V>(divisions, divisions);
  }

  template <class V>
  static WTriangleIndex<V> create() {
    return create<V>(1);
  }
};