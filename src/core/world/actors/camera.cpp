#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/time.h"
#include "core/world/actors/camera.h"
#include "util/util.h"
#include "util/math.h"

void ACamera::setPerspective(float FOV, float aspectRatio, float nearZ,
                             float farZ) noexcept {
  m_viewData.perspectiveData = {FOV, aspectRatio, nearZ, farZ};
  m_projection = glm::perspective(
      glm::radians(m_viewData.perspectiveData.x), m_viewData.perspectiveData.y,
      m_viewData.perspectiveData.z, m_viewData.perspectiveData.w);
}

const glm::vec4& ACamera::getPerspective() noexcept {
  return m_viewData.perspectiveData;
}

void ACamera::setOrthographic(float left, float right, float bottom, float top,
                              float nearZ, float farZ) noexcept {
  m_projection = glm::ortho(left, right, bottom, top, nearZ, farZ);
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

void ACamera::setRotation(float x, float y, float z) noexcept {
  setRotation({glm::radians(x), glm::radians(y), glm::radians(z)});
}

void ACamera::setRotation(const glm::vec3& newRotation) noexcept {
  m_pitch = newRotation.x < -config::pitchLimit  ? -config::pitchLimit
            : newRotation.x > config::pitchLimit ? config::pitchLimit
                                                 : newRotation.x;

  m_transformationData.initial.rotation =
      glm::quat(glm::vec3(m_pitch, newRotation.y, newRotation.z));
  m_transformationData.rotation = m_transformationData.initial.rotation;

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

void ACamera::setRotation(const glm::quat& newRotation) noexcept {
  m_transformationData.rotation = newRotation;

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
      float newPitch = m_pitch + realAngle;

      if (newPitch < -config::pitchLimit) {
        break;
      }

      if (newPitch > config::pitchLimit) {
        break;
      }

      m_pitch = newPitch;
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

void ACamera::setFOV(float FOV) noexcept {
  m_viewData.perspectiveData.x = FOV;
  m_projection = glm::perspective(
      glm::radians(m_viewData.perspectiveData.x), m_viewData.perspectiveData.y,
      m_viewData.perspectiveData.z, m_viewData.perspectiveData.w);
}

void ACamera::setAspectRatio(float ratio) noexcept {
  m_viewData.perspectiveData.y = ratio;
  m_projection = glm::perspective(
      glm::radians(m_viewData.perspectiveData.x), m_viewData.perspectiveData.y,
      m_viewData.perspectiveData.z, m_viewData.perspectiveData.w);
}
