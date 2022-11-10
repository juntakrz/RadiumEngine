#pragma once

#include "pch.h"

template <class V>
class WTriangleIndex {
 public:
  std::vector<V> vertices;
  std::vector<uint32_t> indices;

  WTriangleIndex() = default;
  WTriangleIndex(std::vector<V> inVerts, std::vector<uint32_t> inIndices)
      : vertices(std::move(inVerts)), indices(std::move(inIndices)) {
    ASSERT(vertices.size() > 2);
    ASSERT(indices.size() % 3 == 0);
  }
  /*
  void Transform(DirectX::FXMMATRIX matrix) {
    for (auto& v : vertices) {
      const DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&v.pos);
      DirectX::XMStoreFloat3(&v.pos, DirectX::XMVector3Transform(pos, matrix));
    }
  }

  void SetTangentBinormalNormal() noexcept {
    if (indices.size() % 3 != 0 || indices.size() < 3) {
      return;
    }

    for (uint32_t i = 0; i < indices.size(); i += 3) {
      // load vertices
      auto& vertex0 = vertices[indices[i]];
      auto& vertex1 = vertices[indices[i + 1]];
      auto& vertex2 = vertices[indices[i + 2]];

      // geometry vectors
      const DirectX::XMVECTOR vec0 = DirectX::XMLoadFloat3(&vertex0.pos);
      const DirectX::XMVECTOR vec1 = DirectX::XMLoadFloat3(&vertex1.pos);
      const DirectX::XMVECTOR vec2 = DirectX::XMLoadFloat3(&vertex2.pos);

      const DirectX::XMVECTOR vecSide_1_0 = vec1 - vec0;
      const DirectX::XMVECTOR vecSide_2_0 = vec2 - vec0;

      DirectX::XMFLOAT3 xmSide_1_0, xmSide_2_0;
      DirectX::XMStoreFloat3(&xmSide_1_0, vecSide_1_0);
      DirectX::XMStoreFloat3(&xmSide_2_0, vecSide_2_0);

      // texture vectors
      const DirectX::XMFLOAT2 xmU = {vertex1.tex.x - vertex0.tex.x,
                                     vertex2.tex.x - vertex0.tex.x};
      const DirectX::XMFLOAT2 xmV = {vertex1.tex.y - vertex0.tex.y,
                                     vertex2.tex.y - vertex0.tex.y};

      // denominator of the tangent/binormal equation: 1.0 / cross(vecX, vecY)
      const float den = 1.0f / (xmU.x * xmV.y - xmU.y * xmV.x);

      // calculate tangent
      DirectX::XMFLOAT3 tangent;

      tangent.x = (xmV.y * xmSide_1_0.x - xmV.x * xmSide_2_0.x) * den;
      tangent.y = (xmV.y * xmSide_1_0.y - xmV.x * xmSide_2_0.y) * den;
      tangent.z = (xmV.y * xmSide_1_0.z - xmV.x * xmSide_2_0.z) * den;

      // calculate binormal
      DirectX::XMFLOAT3 binormal;

      binormal.x = (xmU.x * xmSide_2_0.x - xmU.y * xmSide_1_0.x) * den;
      binormal.y = (xmU.x * xmSide_2_0.y - xmU.y * xmSide_1_0.y) * den;
      binormal.z = (xmU.x * xmSide_2_0.z - xmU.y * xmSide_1_0.z) * den;

      // normalize binormal and tangent
      const DirectX::XMVECTOR vecT =
          DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&tangent));
      const DirectX::XMVECTOR vecB =
          DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&binormal));

      // store new data to vertex
      DirectX::XMStoreFloat3(&vertex0.t, vecT);
      DirectX::XMStoreFloat3(&vertex0.b, -vecB);

      DirectX::XMStoreFloat3(&vertex1.t, vecT);
      DirectX::XMStoreFloat3(&vertex1.b, -vecB);

      DirectX::XMStoreFloat3(&vertex2.t, vecT);
      DirectX::XMStoreFloat3(&vertex2.b, -vecB);

      SetNormals();
    }
  }

  void SetNormalsTri() noexcept {
    // ASSERT(indices.size() % 3 == 0 && indices.size() > 2);
    if (indices.size() % 3 != 0 || indices.size() < 3) {
      return;
    }

    for (uint32_t i = 0; i < indices.size(); i += 3) {
      auto& vertex0 = vertices[indices[i]];
      auto& vertex1 = vertices[indices[i + 1]];
      auto& vertex2 = vertices[indices[i + 2]];

      const DirectX::XMVECTOR vec0 = DirectX::XMLoadFloat3(&vertex0.pos);
      const DirectX::XMVECTOR vec1 = DirectX::XMLoadFloat3(&vertex1.pos);
      const DirectX::XMVECTOR vec2 = DirectX::XMLoadFloat3(&vertex2.pos);

      // cross product of triangle's side vectors calculated by connecting
      // vertices by vector
      const auto normal = DirectX::XMVector3Normalize(
          DirectX::XMVector3Cross((vec1 - vec0), (vec2 - vec0)));

      DirectX::XMStoreFloat3(&vertex0.n, normal);
      DirectX::XMStoreFloat3(&vertex1.n, normal);
      DirectX::XMStoreFloat3(&vertex2.n, normal);
    }
  }

  void SetNormals() noexcept {
    for (auto& it : vertices) {
      it.n = it.pos;
    }
  }

  */
};