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
  m_projectionType = ECameraProjection::Perspective;
}

const glm::vec4& ACamera::getPerspective() noexcept {
  return m_viewData.perspectiveData;
}

void ACamera::setOrthographic(float horizontal, float vertical, float nearZ,
                              float farZ) noexcept {
  m_viewData.orthographicData = {horizontal, vertical, nearZ, farZ};
  m_projection = glm::ortho(-horizontal, horizontal, -vertical, vertical, nearZ, farZ);
  m_projectionType = ECameraProjection::Orthogtaphic;
}

glm::vec2 ACamera::getNearAndFarPlane() {
  switch (m_projectionType) {
    case ECameraProjection::Perspective:
      return { m_viewData.perspectiveData.z, m_viewData.perspectiveData.w };
    case ECameraProjection::Orthogtaphic:
      return { m_viewData.orthographicData.z, m_viewData.orthographicData.w };
  }
}

glm::mat4& ACamera::getView() {
  return m_view = glm::lookAt(
             m_transformationData.translation,
             m_transformationData.translation + m_transformationData.forwardVector,
             m_viewData.upVector);
}

glm::mat4& ACamera::getProjection() { return m_projection; }

const glm::mat4& ACamera::getProjectionView() {
  if (m_transformationData.wasUpdated) {
    m_projectionView = getProjection() * getView();
    m_transformationData.wasUpdated = false;
  }

  return m_projectionView;
}

ECameraProjection ACamera::getProjectionType() { return m_projectionType; }

void ACamera::setUpVector(float x, float y, float z) noexcept {
  m_viewData.upVector = {x, y, z};
}

void ACamera::setUpVector(const glm::vec3& upVector) noexcept {
  m_viewData.upVector = upVector;
}

void ACamera::setLookAtTarget(ABase* pTarget, const bool useForwardVector,
                              const bool attach) noexcept {
  if (!pTarget) {
    RE_LOG(Error,
           "Failed to set lookAt target for '%s'. No target was provided.",
           m_name.c_str());
    return;
  }

  m_transformationData.forwardVector = pTarget->getLocation() - m_transformationData.translation;

  if (attach) {
    attachTo(pTarget, true, false, true);
  }
}

void ACamera::translate(const glm::vec3& delta) noexcept {
  glm::vec3 moveDirection = glm::rotate(m_transformationData.rotation, delta);
  m_transformationData.translation +=
      moveDirection * m_translationModifier * core::time.getDeltaTime();

  updateAttachments();
}

void ACamera::setRotation(float x, float y, float z) noexcept {
  setRotation({glm::radians(x), glm::radians(y), glm::radians(z)});
}

void ACamera::setRotation(const glm::vec3& newRotation) noexcept {
  m_pitch = m_viewData.ignorePitchLimit ? newRotation.x :
    newRotation.x < -config::pitchLimit  ? -config::pitchLimit
            : newRotation.x > config::pitchLimit ? config::pitchLimit
                                                 : newRotation.x;

  m_transformationData.initial.rotation =
      glm::quat(glm::vec3(m_pitch, newRotation.y, newRotation.z));
  m_transformationData.rotation = m_transformationData.initial.rotation;

  switch (m_viewData.anchorFocusPoint) {
    case true: {
      // code for when the camera should be rotated around its focus point
      break;
    }
    case false: {
      m_transformationData.forwardVector = glm::rotate(m_transformationData.rotation,
                                          m_transformationData.initial.forwardVector);
    }
  }

  updateAttachments();
}

void ACamera::setRotation(const glm::quat& newRotation) noexcept {
  m_transformationData.rotation = newRotation;

  switch (m_viewData.anchorFocusPoint) {
    case true: {
      // code for when the camera should be rotated around its focus point
      break;
    }
    case false: {
      m_transformationData.forwardVector =
          glm::rotate(m_transformationData.rotation,
                      m_transformationData.initial.forwardVector);
    }
  }

  updateAttachments();
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

      if (!m_viewData.ignorePitchLimit) {
        if (newPitch < -config::pitchLimit) {
          break;
        }

        if (newPitch > config::pitchLimit) {
          break;
        }
      }

      m_pitch = newPitch;
    }
    default: {
      m_transformationData.rotation *= glm::angleAxis(realAngle, vector);
      break;
    }
  }

  switch (m_viewData.anchorFocusPoint) {
    case true: {
      // code for when the camera should be rotated around its focus point
      break;
    }
    case false: {
      m_transformationData.forwardVector =
          glm::rotate(m_transformationData.rotation,
                      m_transformationData.initial.forwardVector);
    }
  }

  updateAttachments();
}

void ACamera::setIgnorePitchLimit(const bool newValue) {
  m_viewData.ignorePitchLimit = newValue;
}

void ACamera::setFOV(float FOV) noexcept {
  if (m_projectionType == ECameraProjection::Orthogtaphic) return;

  m_viewData.perspectiveData.x = FOV;
  m_projection = glm::perspective(
      glm::radians(m_viewData.perspectiveData.x), m_viewData.perspectiveData.y,
      m_viewData.perspectiveData.z, m_viewData.perspectiveData.w);
}

void ACamera::setAspectRatio(float ratio) noexcept {
  if (m_projectionType == ECameraProjection::Orthogtaphic) return;

  m_viewData.perspectiveData.y = ratio;
  m_projection = glm::perspective(
      glm::radians(m_viewData.perspectiveData.x), m_viewData.perspectiveData.y,
      m_viewData.perspectiveData.z, m_viewData.perspectiveData.w);
}

void ACamera::setViewBufferIndex(const uint32_t newIndex) {
  m_viewBufferIndex = newIndex;
}

const uint32_t ACamera::getViewBufferIndex() { return m_viewBufferIndex; }
