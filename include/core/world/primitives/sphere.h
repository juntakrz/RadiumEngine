#pragma once

#include "triangleindex.h"

class WSphere {
 public:
  template <class V>
  static WTriangleIndex<V> create(uint16_t divisions, bool invertFaces) {
    constexpr float radius = 1.0f;

    (divisions > 2) ? 0 : divisions = 3;

    // generate vertices
    std::vector<V> vertices;

    const uint16_t latSegments = divisions;
    const uint16_t longSegments = divisions * 2;

    float u, v, latAngle, longAngle;
    float dX, dY, dZ, dXZ;

    for (uint16_t i = 0; i <= latSegments; i++) {
      v = 1 - (float)i / (float)latSegments;

      latAngle =
          (float(i) * M_PI / float(latSegments)) - M_PI_2;
      dY = sinf(latAngle);
      dXZ = cosf(latAngle);

      for (uint16_t j = 0; j <= longSegments; j++) {
        longAngle = (float)j * (M_PI * 2) / (float)longSegments;
        u = (float)j / (float)longSegments;
        dX = sinf(longAngle) * dXZ;
        dZ = cosf(longAngle) * dXZ;

        vertices.emplace_back();
        vertices.back().pos = {dX, dY, dZ};
        vertices.back().tex = {u, v};
      }
    }

    // generate indices
    std::vector<uint32_t> indices;

    const uint16_t stride = longSegments + 1;
    uint16_t nextI, nextJ;

    for (uint16_t i = 0; i < latSegments; i++) {
      for (uint16_t j = 0; j < longSegments; j++) {
        nextI = i + 1;
        nextJ = (j + 1) % stride;

        indices.emplace_back(i * stride + nextJ);
        indices.emplace_back(nextI * stride + j);
        indices.emplace_back(i * stride + j);

        indices.emplace_back(nextI * stride + nextJ);
        indices.emplace_back(nextI * stride + j);
        indices.emplace_back(i * stride + nextJ);
      }
    }

    ASSERT(indices.size() % 3 == 0);

    // invert faces if set
    if (invertFaces) {
      for (auto it = indices.begin(); it != indices.end(); it += 3) {
        std::swap(*it, *(it + 2));
      }
    }

    return {std::move(vertices), std::move(indices)};
  }

  template <class V>
  static WTriangleIndex<V> create(const uint16_t divisions) {
    return Create<V>(divisions, false);
  }

  template <class V>
  static WTriangleIndex<V> create() {
    return Create<V>(32, false);
  }
};
