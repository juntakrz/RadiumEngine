#include "pch.h"
#include "util/math.h"

glm::mat4 math::interpolate(const glm::mat4& first, const glm::mat4& second,
                            const float coefficient) {
  glm::mat4 outMatrix = glm::mat4(1.0f);
  const float multiplier[4] = {coefficient, coefficient, coefficient,
                               coefficient};

  __m128 simdMultiplier = _mm_load_ps(multiplier);

  for (int i = 0; i < 4; ++i) {
    __m128 nPass = _mm_fnmadd_ps(simdMultiplier, first[i].data, first[i].data);
    __m128 outPass = _mm_fmadd_ps(simdMultiplier, second[i].data, nPass);

    outMatrix[i].data = outPass;
  }

  return outMatrix;
}

void math::interpolate(const glm::mat4& first, const glm::mat4& second,
                       const float coefficient, glm::mat4& outMatrix) {
  const float multiplier[4] = {coefficient, coefficient, coefficient,
                               coefficient};

  __m128 simdMultiplier = _mm_load_ps(multiplier);

  for (int i = 0; i < 4; ++i) {
    __m128 passA = _mm_fnmadd_ps(simdMultiplier, first[i].data, first[i].data);
    __m128 outPass = _mm_fmadd_ps(simdMultiplier, second[i].data, passA);

    outMatrix[i].data = outPass;
  }
}

float math::random(float min, float max) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<float> dist(min, max);
  return dist(mt);
}

uint32_t math::getMipLevels(uint32_t dimension) {
  return static_cast<uint32_t>(floor(log2(dimension))) + 1;
}
