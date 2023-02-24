#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/mtime.h"
#include "core/world/actors/acamera.h"
#include "util/util.h"

void ACamera::setPerspective(float FOV, float aspectRatio, float nearZ,
                               float farZ) noexcept {
  m_projection = glm::perspective(FOV, aspectRatio, nearZ, farZ);
}

glm::mat4& ACamera::getView() {
  return m_view = glm::lookAt(m_transformData.translation, m_transformData.translation + m_viewData.focusPoint, m_viewData.upVector);
}

glm::mat4& ACamera::getProjection() { return m_projection; }

void ACamera::setUpVector(const glm::vec3& upVector) {
  m_viewData.upVector = upVector;
}

void ACamera::setUpVector(float x, float y, float z) {
  m_viewData.upVector = {x, y, z};
}

void ACamera::rotate(float x, float y, float z) noexcept {
  m_transformData.rotation = glm::mod(
      m_transformData.rotation +
          (glm::vec3(x, y, z) * m_rotationSpeed * core::time.getDeltaTime()),
      glm::two_pi<float>());

  // ISSUE: currently camera rotation is happening additively, adding every new value to an old value
  // instead of rotating by only the new value
  RE_LOG(Log, "%s: rotation: %.2f %.2f %.2f input: %.2f %.2f %.2f /",
         __FUNCTION__, m_transformData.rotation.x, m_transformData.rotation.y,
         m_transformData.rotation.z, x, y, z);

  switch (m_viewData.bAnchorFocusPoint) {
    case true: {
      //
      break;
    }
    case false: {
      /*m_viewData.focusPoint = glm::rotate(
          m_viewData.focusPoint, m_rotationSpeed * core::time.getDeltaTime(),
          m_transformData.rotation);*/
      m_viewData.focusPoint = glm::vec3(0.0f, 0.0f, 1.0f);      // bad fix of the ISSUE, do better
      m_viewData.focusPoint =
          glm::rotateX(m_viewData.focusPoint, m_transformData.rotation.x);
      m_viewData.focusPoint =
          glm::rotateY(m_viewData.focusPoint, m_transformData.rotation.y);
      break;
    }
  }
}