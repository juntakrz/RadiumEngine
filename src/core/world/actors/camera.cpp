#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/time.h"
#include "core/world/actors/camera.h"
#include "util/util.h"

void ACamera::setPerspective(float FOV, float aspectRatio, float nearZ,
                               float farZ) noexcept {
  m_projection = glm::perspective(FOV, aspectRatio, nearZ, farZ);
}

glm::mat4& ACamera::getView() {
  return m_view = glm::lookAt(m_transformData.translation, m_transformData.translation + m_viewData.focusPoint, m_viewData.upVector);
}

glm::mat4& ACamera::getProjection() { return m_projection; }

void ACamera::setUpVector(float x, float y, float z) noexcept {
  m_viewData.upVector = {x, y, z};
}

void ACamera::setUpVector(const glm::vec3& upVector) noexcept {
  m_viewData.upVector = upVector;
}

void ACamera::rotate(float x, float y, float z) noexcept {
  m_transformData.rotation = glm::mod(
      m_transformData.rotation +
          (glm::vec3(x, y, z) * m_rotationSpeed * core::time.getDeltaTime()),
      glm::two_pi<float>());

  switch (m_viewData.bAnchorFocusPoint) {
    case true: {
      // code for when the camera should be rotated around its focus point
      break;
    }
    case false: {
      m_viewData.focusPoint = glm::rotateX(
          glm::rotateY(m_transformData.frontVector, m_transformData.rotation.y),
          m_transformData.rotation.x);
      break;
    }
  }
}

void ACamera::translate(float x, float y, float z) noexcept {
  glm::vec3 moveDirection =
      glm::rotateX(glm::rotateY(glm::vec3(x, y, z), m_transformData.rotation.y), m_transformData.rotation.x);
  m_transformData.translation += moveDirection * m_translationSpeed * core::time.getDeltaTime();
}
