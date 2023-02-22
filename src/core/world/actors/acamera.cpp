#include "pch.h"
#include "core/objects.h"
#include "core/world/actors/acamera.h"

void ACamera::setPerspective(float FOV, float aspectRatio, float nearZ,
                               float farZ) noexcept {
  m_projection = glm::perspective(FOV, aspectRatio, nearZ, farZ);
}

glm::mat4 ACamera::view() {
  return m_view = glm::lookAtLH(transform.translation, m_center, m_up);
}

void ACamera::setUpVector(const glm::vec3& upVector) { m_up = upVector; }

void ACamera::setUpVector(float x, float y, float z) {
  m_up = {x, y, z};
}
