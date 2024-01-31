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

void math::getHaltonJitter(std::vector<glm::vec2>& outVector, const int32_t width, const int32_t height) {
  const int32_t valueCount = static_cast<int32_t>(outVector.size());
  const float widthCoef = 1.0f / (float)width;
  const float heightCoef = 1.0f / (float)height;

  auto fGetHaltonValue = [](int32_t index, int32_t base) {
    float f = 1;
    float r = 0;
    int current = index;
    do
    {
      f = f / base;
      r = r + f * (current % base);
      current = static_cast<int32_t>(glm::floor(current / base));
    } while (current > 0);
    return r;
  };

  for (int32_t i = 0; i < valueCount; ++i) {
    outVector[i] = {fGetHaltonValue(i + 1, 2), fGetHaltonValue(i + 1, 3)};
    outVector[i] = {(outVector[i].x * 2.0f - 1.0f) * widthCoef, (outVector[i].y * 2.0f - 1.0f) * heightCoef};
  }
}