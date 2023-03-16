#pragma once

#include "primitivedata.h"

class WPrimitiveGen_Cube {
 public:
  template <typename V>
  static WPrimitiveData<V> create(int32_t divisions, bool unused) {
    constexpr float coord = 1.0f;

    std::vector<V> vertices(24);
    std::vector<uint32_t> indices;

    // front face
    vertices[0].pos = {-coord, -coord, -coord};  // BL -> TL -> TR -> BR
    vertices[1].pos = {-coord, coord, -coord};
    vertices[2].pos = {coord, coord, -coord};
    vertices[3].pos = {coord, -coord, -coord};

    // top face
    vertices[4].pos = {-coord, coord, -coord};
    vertices[5].pos = {-coord, coord, coord};
    vertices[6].pos = {coord, coord, coord};
    vertices[7].pos = {coord, coord, -coord};

    // left face
    vertices[8].pos = {-coord, -coord, coord};
    vertices[9].pos = {-coord, coord, coord};
    vertices[10].pos = {-coord, coord, -coord};
    vertices[11].pos = {-coord, -coord, -coord};

    // rear face
    vertices[12].pos = {coord, -coord, coord};
    vertices[13].pos = {coord, coord, coord};
    vertices[14].pos = {-coord, coord, coord};
    vertices[15].pos = {-coord, -coord, coord};

    // right face
    vertices[16].pos = {coord, -coord, -coord};
    vertices[17].pos = {coord, coord, -coord};
    vertices[18].pos = {coord, coord, coord};
    vertices[19].pos = {coord, -coord, coord};

    // bottom face
    vertices[20].pos = {-coord, -coord, coord};
    vertices[21].pos = {-coord, -coord, -coord};
    vertices[22].pos = {coord, -coord, -coord};
    vertices[23].pos = {coord, -coord, coord};

    for (uint8_t i = 0; i < 24; i += 4) {
      indices.emplace_back(i);
      indices.emplace_back(i + 1);
      indices.emplace_back(i + 2);

      indices.emplace_back(i);
      indices.emplace_back(i + 2);
      indices.emplace_back(i + 3);

      vertices[i].tex0 = {0.0f, 1.0f};
      vertices[i + 1].tex0 = {0.0f, 0.0f};
      vertices[i + 2].tex0 = {1.0f, 0.0f};
      vertices[i + 3].tex0 = {1.0f, 1.0f};
    }

    return {std::move(vertices), std::move(indices)};
  }

  // AXIS ALIGNED BOUNDING BOX

  template <class V>
  static WPrimitiveData<V> create(glm::vec3 vMin, glm::vec3 vMax) {
    // increase bounding box size for safer occlusion testing
    const float bias = 0.001f;

    vMin.x -= bias;
    vMin.y -= bias;
    vMin.z -= bias;

    vMax.x += bias;
    vMax.y += bias;
    vMax.z += bias;

    std::vector<V> vertices(24);
    std::vector<uint32_t> indices;

    // front face
    vertices[0].pos = {vMin.x, vMin.y, vMin.z};  // BL -> TL -> TR -> BR
    vertices[1].pos = {vMin.x, vMax.y, vMin.z};
    vertices[2].pos = {vMax.x, vMax.y, vMin.z};
    vertices[3].pos = {vMax.x, vMin.y, vMin.z};

    // top face
    vertices[4].pos = {vMin.x, vMax.y, vMin.z};
    vertices[5].pos = {vMin.x, vMax.y, vMax.z};
    vertices[6].pos = {vMax.x, vMax.y, vMax.z};
    vertices[7].pos = {vMax.x, vMax.y, vMin.z};

    // left face
    vertices[8].pos = {vMin.x, vMin.y, vMax.z};
    vertices[9].pos = {vMin.x, vMax.y, vMax.z};
    vertices[10].pos = {vMin.x, vMax.y, vMin.z};
    vertices[11].pos = {vMin.x, vMin.y, vMin.z};

    // rear face
    vertices[12].pos = {vMax.x, vMin.y, vMax.z};
    vertices[13].pos = {vMax.x, vMax.y, vMax.z};
    vertices[14].pos = {vMin.x, vMax.y, vMax.z};
    vertices[15].pos = {vMin.x, vMin.y, vMax.z};

    // right face
    vertices[16].pos = {vMax.x, vMin.y, vMin.z};
    vertices[17].pos = {vMax.x, vMax.y, vMin.z};
    vertices[18].pos = {vMax.x, vMax.y, vMax.z};
    vertices[19].pos = {vMax.x, vMin.y, vMax.z};

    // bottom face
    vertices[20].pos = {vMin.x, vMin.y, vMax.z};
    vertices[21].pos = {vMin.x, vMin.y, vMin.z};
    vertices[22].pos = {vMax.x, vMin.y, vMin.z};
    vertices[23].pos = {vMax.x, vMin.y, vMax.z};

    for (uint8_t i = 0; i < 24; i += 4) {
      indices.emplace_back(i);
      indices.emplace_back(i + 1);
      indices.emplace_back(i + 2);

      indices.emplace_back(i);
      indices.emplace_back(i + 2);
      indices.emplace_back(i + 3);

      vertices[i].tex = {0.0f, 1.0f};
      vertices[i + 1].tex = {0.0f, 0.0f};
      vertices[i + 2].tex = {1.0f, 0.0f};
      vertices[i + 3].tex = {1.0f, 1.0f};
    }

    return {std::move(vertices), std::move(indices)};
  }
};