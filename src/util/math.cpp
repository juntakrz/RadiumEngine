#include "pch.h"
#include "util/math.h"

glm::mat4 math::interpolate(const glm::mat4& first, const glm::mat4& second,
                            const float coefficient) {
  glm::mat4 outMatrix = glm::mat4(1.0f);
  const float multiplier[4] = {coefficient, coefficient, coefficient,
                               coefficient};

  __m128 simdMultiplier = _mm_load_ps(multiplier);

  for (int8_t i = 0; i < 4; ++i) {
    __m128 nPass = _mm_fnmadd_ps(simdMultiplier, first[i].data, first[i].data);
    __m128 outPass = _mm_fmadd_ps(simdMultiplier, second[i].data, nPass);
    _mm_store_ps((float*)&outMatrix[i], outPass);
  }

  return outMatrix;
}

void math::interpolate(const glm::mat4& first, const glm::mat4& second,
                       const float coefficient, glm::mat4& outMatrix) {
  const float multiplier[4] = {coefficient, coefficient, coefficient,
                               coefficient};

  __m128 simdMultiplier = _mm_load_ps(multiplier);

  for (int8_t i = 0; i < 4; ++i) {
    __m128 passA = _mm_fnmadd_ps(simdMultiplier, first[i].data, first[i].data);
    __m128 outPass = _mm_fmadd_ps(simdMultiplier, second[i].data, passA);
    _mm_store_ps((float*)&outMatrix[i], outPass);
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

bool math::decomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale) {
  using T = float;

	glm::mat4 localMatrix(transform);

	// Normalize the matrix
	if (glm::epsilonEqual(localMatrix[3][3], static_cast<float>(0), glm::epsilon<T>()))
		return false;

	// Isolate perspective
	if (
    glm::epsilonNotEqual(localMatrix[0][3], static_cast<T>(0), glm::epsilon<T>()) ||
    glm::epsilonNotEqual(localMatrix[1][3], static_cast<T>(0), glm::epsilon<T>()) ||
    glm::epsilonNotEqual(localMatrix[2][3], static_cast<T>(0), glm::epsilon<T>())) {
		// Clear the perspective partition
		localMatrix[0][3] = localMatrix[1][3] = localMatrix[2][3] = static_cast<T>(0);
		localMatrix[3][3] = static_cast<T>(1);
	}

	// Translation
  outTranslation = glm::vec3(localMatrix[3]);
	localMatrix[3] = glm::vec4(0, 0, 0, localMatrix[3].w);

  glm::vec3 row[3];

	// Get scale and shear
	for (glm::length_t i = 0; i < 3; ++i)
		for (glm::length_t j = 0; j < 3; ++j)
			row[i][j] = localMatrix[i][j];

	// Compute X scale factor and normalize first row
	outScale.x = glm::length(row[0]);
	row[0] = glm::detail::scale(row[0], static_cast<T>(1));
	outScale.y = glm::length(row[1]);
	row[1] = glm::detail::scale(row[1], static_cast<T>(1));
	outScale.z = glm::length(row[2]);
	row[2] = glm::detail::scale(row[2], static_cast<T>(1));

	outRotation.y = asin(-row[0][2]);
	if (cos(outRotation.y) != 0) {
		outRotation.x = atan2(row[1][2], row[2][2]);
		outRotation.z = atan2(row[0][1], row[0][0]);
	}
	else {
		outRotation.x = atan2(-row[2][0], row[1][1]);
		outRotation.z = 0;
	}


	return true;
}