#include "pch.h"
#include "util/math.h"

glm::mat4 math::interpolate(const glm::mat4& first, const glm::mat4& second,
                            const float coefficient) {
  glm::mat4 outMatrix = glm::mat4(1.0f);
  const float multiplier[4] = {coefficient, coefficient, coefficient,
                               coefficient};

  __m128 simdMultiplier = _mm_load_ps(multiplier);

  for (int i = 0; i < 4; ++i) {
    __m128 vectorA = _mm_load_ps((float*)&first[i]);
    __m128 vectorB = _mm_load_ps((float*)&second[i]);

    __m128 passA = _mm_fnmadd_ps(simdMultiplier, vectorA, vectorA);
    __m128 outPass = _mm_fmadd_ps(simdMultiplier, vectorB, passA);

    memcpy(&outMatrix[i], outPass.m128_f32, 4 * sizeof(float));
  }

  return outMatrix;
}
