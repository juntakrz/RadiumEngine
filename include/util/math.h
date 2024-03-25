#pragma once

namespace math {
constexpr double PI64 = 3.1415926535897932384626433832795028841971693993751;
constexpr float PI = static_cast<float>(PI64);
constexpr double twoPI64 = PI64 * 2.0;
constexpr float twoPI = static_cast<float>(PI64 * 2.0f);
constexpr double halfPI64 = PI64 / 2.0;
constexpr float halfPI =  static_cast<float>(PI64 / 2.0f);
constexpr float SIMD_PI[4] = { math::PI, math::PI, math::PI, math::PI };
constexpr float SIMD_2PI[4] = { math::twoPI, math::twoPI, math::twoPI, math::twoPI };

template <typename T>
inline void wrapAngle(T& angle) {
  constexpr T PI = (T)math::PI64;
  constexpr T PI2 = (T)math::twoPI64;

  angle += PI;
  angle = fmod(angle, PI2);
  if (angle < (T)0.0) angle += PI2;
  angle -= PI;
}

// Wrap angles of GLM vector to -180 .. 180 degrees (-PI .. PI)
template <typename T>
inline void wrapAnglesGLM(T& vector) {
#if defined(GLM_FORCE_AVX) || defined(GLM_FORCE_SSE2)
  __m128 mmVector = _mm_load_ps(vector.data.data);
  __m128 mmPI = _mm_load_ps(SIMD_PI);
  __m128 mm2PI = _mm_load_ps(SIMD_2PI);
  mmVector = _mm_add_ps(mmVector, mmPI);
  __m128 mmResult = _mm_floor_ps(_mm_div_ps(mmVector, mm2PI));
  mmResult = _mm_mul_ps(mm2PI, mmResult);
  mmResult = _mm_sub_ps(mmVector, mmResult);

  const int8_t count = vector.length();
  for (int8_t i = 0; i < count; ++i) {
    if (mmResult.m128_f32[i] < 0.0f) mmResult.m128_f32[i] += math::twoPI;
  }

  mmResult = _mm_sub_ps(mmResult, mmPI);

  memcpy(vector.data.data, mmResult.m128_f32, sizeof(float) * count);
#else
  const int8_t count = vector.length();
  for (int8_t i = 0; i < count; ++i) {
    vector.data.data[i] += math::PI;
    vector.data.data[i] = fmod(vector.data.data[i], math::twoPI);
    if (vector.data.data[i] < 0.0f) vector.data.data[i] += math::twoPI;
    vector.data.data[i] -= math::PI;
  }
#endif
}

template <typename T>
inline T normalizeMouseCoord(const T coord, const T dimension) {
  return (dimension > 0) ? coord / dimension * (T)2.0 - (T)1.0
                         : (coord / dimension * (T)2.0) + (T)1.0;
}

template <typename T>
constexpr T gaussDistribution(
    const T& x,
    const T& sigma) noexcept  // Gaussian distribution formula for blur
{
  const auto ss = sigma * sigma;
  return ((T)1.0 / sqrt((T)2.0 * (T)PI64 * ss)) * exp(-(x * x) / ((T)2.0 * ss));
}

template <typename T>
constexpr std::vector<T> gaussBlurKernel(uint8_t radius, T sigma) noexcept {
  // limit kernel radius to 3 .. 8 (gauss blur shader limitation)
  (radius > 8) ? radius = 8 : (radius < 3) ? radius = 3 : radius;

  // initialize data
  std::vector<T> gaussCoef;
  T sum = T(0.0);
  T core = T(0.0);

  // padding lambda
  auto emplace = [&](const T& var) {
    gaussCoef.emplace_back(var);
    for (uint8_t p = 0; p < 3; p++) {
      gaussCoef.emplace_back();
    }
  };

  // insert radius as the first variable
  emplace(T(radius));

  for (int8_t i = radius; i > -1; i--) {
    const T x = T(i - radius);
    const T g = math::gaussDistribution(x, sigma);
    (i != radius) ? sum += g : core = g;

    // emplace coefficient
    emplace(g);
  }

  // calculate final sum (everything but core repeats twice)
  sum = sum * T(2.0) + core;

  // normalize coefficients
  uint8_t steppedRadius = radius * 4;
  for (uint8_t j = 4; j < steppedRadius + 1; j += 4) {
    gaussCoef[j] /= sum;
  }

  return gaussCoef;
}

// interpolate two 4x4 matrices, function uses FMA3 instruction set
glm::mat4 interpolate(const glm::mat4& first, const glm::mat4& second,
                              const float coefficient);

// interpolate two 4x4 matrices, writes directly to external matrix
void interpolate(const glm::mat4& first, const glm::mat4& second,
                      const float coefficient, glm::mat4& outMatrix);

float random(float min, float max);

// calculate how many mip levels a square texture may fit
uint32_t getMipLevels(uint32_t dimension);

void getHaltonJitter(std::vector<glm::vec2>& outVector, const int32_t width, const int32_t height);

bool decomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);

}  // namespace math