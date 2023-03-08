#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/time.h"
#include "core/world/actors/camera.h"
#include "util/util.h"
#include "util/math.h"

void ACamera::setPerspective(float FOV, float aspectRatio, float nearZ,
                               float farZ) noexcept {
  m_projection = glm::perspective(FOV, aspectRatio, nearZ, farZ);
}

glm::mat4& ACamera::getView() {
  return m_view = glm::lookAt(
             m_transformationData.translation,
             m_transformationData.translation + m_viewData.focusPoint,
             m_viewData.upVector);
}

glm::mat4& ACamera::getProjection() { return m_projection; }

void ACamera::setUpVector(float x, float y, float z) noexcept {
  m_viewData.upVector = {x, y, z};
}

void ACamera::setUpVector(const glm::vec3& upVector) noexcept {
  m_viewData.upVector = upVector;
}

void ACamera::translate(const glm::vec3& delta) noexcept {
  glm::vec3 moveDirection = glm::rotate(m_transformationData.rotation, delta);
  m_transformationData.translation +=
      moveDirection * m_translationModifier * core::time.getDeltaTime();
}

void ACamera::rotate(const glm::vec3& vector, float angle) noexcept {
  // when rotating around an axis similar to camera's up vector a pre-multiplication must be done
  // e.g. rotation = modifier * rotation
  uint8_t direction = vector.x > 0.0f ? 0 : vector.y > 0.0f ? 1 : 2;
  float realAngle = angle * m_rotationModifier * core::time.getDeltaTime();

  switch (direction) {
    case 1: {
      m_transformationData.rotation =
          glm::angleAxis(realAngle, vector) * m_transformationData.rotation;
      break;
    }
    case 0: {
      float newPitch = glm::pitch(m_transformationData.rotation) + realAngle;
      if (newPitch > config::pitchLimit) {
        break;
      }
      if (newPitch < -config::pitchLimit) {
        break;
      }
    }
    default: {
      m_transformationData.rotation *= glm::angleAxis(realAngle, vector);
      break;
    }
  }

  switch (m_viewData.bAnchorFocusPoint) {
    case true: {
      // code for when the camera should be rotated around its focus point
      break;
    }
    case false: {
      m_viewData.focusPoint = glm::rotate(m_transformationData.rotation,
                                          m_transformationData.frontVector);
    }
  }
}