#include "pch.h"
#include "util/math.h"

glm::mat4 math::interpolate(const glm::mat4& first,
                                    const glm::mat4& second,
                                    const float coefficient) {
  glm::quat firstRotation = glm::quat_cast(first);
  glm::quat secondRotation = glm::quat_cast(second);
  glm::quat finalRotation = glm::slerp(firstRotation, secondRotation, coefficient);
  glm::mat4 finalMatrix = glm::mat4_cast(finalRotation);
  finalMatrix[3] = first[3] * (1 - coefficient) + second[3] * coefficient;

  return finalMatrix;
}
