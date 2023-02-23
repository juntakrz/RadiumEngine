#include "pch.h"
#include "core/objects.h"
#include "core/world/actors/acamera.h"

void ACamera::setPerspective(float FOV, float aspectRatio, float nearZ,
                               float farZ) noexcept {
  m_projection = glm::perspective(FOV, aspectRatio, nearZ, farZ);
}

glm::mat4& ACamera::getView() {
  return m_view = glm::lookAt(transform.translation, transform.translation + m_viewData.focusPoint, m_viewData.upVector);
}

glm::mat4& ACamera::getProjection() { return m_projection; }

void ACamera::setUpVector(const glm::vec3& upVector) { m_viewData.upVector = upVector; }

void ACamera::setUpVector(float x, float y, float z) {
  m_viewData.upVector = {x, y, z};
}
