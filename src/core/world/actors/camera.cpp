#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "util/util.h"
#include "util/math.h"
#include "core/managers/time.h"
#include "core/model/primitive.h"
#include "core/world/components/components.h"
#include "core/world/actors/camera.h"

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

float ACamera::getNearPlane() {
  return (m_projectionType == ECameraProjection::Perspective)
    ? m_viewData.perspectiveData.z : m_viewData.orthographicData.z;
}

float ACamera::getFarPlane() {
  return (m_projectionType == ECameraProjection::Perspective)
    ? m_viewData.perspectiveData.w : m_viewData.orthographicData.w;
}

glm::vec2 ACamera::getNearAndFarPlane() {
  return (m_projectionType == ECameraProjection::Perspective)
    ? glm::vec2(m_viewData.perspectiveData.z, m_viewData.perspectiveData.w) 
      : glm::vec2(m_viewData.orthographicData.z, m_viewData.orthographicData.w);
}

glm::mat4& ACamera::getView(const bool update) {
  auto& data = getComponent<WTransformComponent>()->data;

  return (update)
    ? m_view = glm::lookAt(data.translation, data.translation + data.forwardVector, m_viewData.upVector)
    : m_view;
}

glm::mat4& ACamera::getProjection() { return m_projection; }

const glm::mat4& ACamera::getProjectionView() {
  auto& data = getComponent<WTransformComponent>()->data;

  if (data.wasUpdated) {
    m_projectionView = getProjection() * getView();
    data.wasUpdated = false;
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
  auto& data = getComponent<WTransformComponent>()->data;

  data.forwardVector = pTarget->getTranslation() - data.translation;

  if (attach) {
    attachTo(pTarget, true, false, true);
  }
}

void ACamera::setTranslation(float x, float y, float z, bool isDelta) noexcept {
  setTranslation(glm::vec3(x, y, z), isDelta);
}

void ACamera::setTranslation(const glm::vec3& newTranslation, bool isDelta) noexcept {
  WTransformComponent* pComponent = getComponent<WTransformComponent>();

  switch (isDelta) {
    case true: {
      glm::vec3 moveDirection = glm::rotate(pComponent->getOrientation(), newTranslation);
      pComponent->setTranslation(moveDirection, true);
      break;
    }

    case false: {
      pComponent->setTranslation(newTranslation, false);
      break;
    }
  }

  updateAttachments();
}

void ACamera::setRotation(const glm::vec3& newRotation, bool isInRadians, bool isDelta) noexcept {
  WTransformComponent* pComponent = getComponent<WTransformComponent>();
  glm::vec3 rotation = (isInRadians) ? newRotation : glm::radians(newRotation);

  switch (isDelta) {
    case true: {
      bool isYaw = rotation.y != 0.0f ? true : false;

      float newPitch = rotation.x + pComponent->getRotation().x;
      if (!m_viewData.ignorePitchLimit && (newPitch < -config::pitchLimit || newPitch > config::pitchLimit)) {
        break;
      }

      pComponent->data.rotation += rotation;
      pComponent->data.orientation = (isYaw)
        ? glm::quat(rotation) * pComponent->data.orientation
        : pComponent->data.orientation * glm::quat(rotation);
      break;
    }

    case false: {
      float newPitch = m_viewData.ignorePitchLimit ? rotation.x :
        rotation.x < -config::pitchLimit ? -config::pitchLimit
        : rotation.x > config::pitchLimit ? config::pitchLimit
        : rotation.x;

      pComponent->setRotation(glm::vec3(newPitch, rotation.y, rotation.z), true, false);
      break;
    }
  }

  switch (m_viewData.anchorFocusPoint) {
    case true: {
      // code for when the camera should be rotated around its focus point
      break;
    }
    case false: {
      pComponent->setForwardVector(
        glm::rotate(pComponent->getOrientation(), pComponent->getAbsoluteForwardVector()));
    }
  }

  updateAttachments();
}

//void ACamera::rotate(const glm::vec3& vector, float angle) noexcept {
//  // When rotating around an axis similar to camera's up vector a pre-multiplication must be done
//  // e.g. rotation = modifier * rotation
//  auto& data = getComponent<WTransformComponent>()->data;
//  uint8_t direction = vector.x > 0.0f ? 0 : vector.y > 0.0f ? 1 : 2;
//  float realAngle = angle * m_rotationModifier * core::time.getDeltaTime();
//
//  switch (direction) {
//    case 1: {
//      glm::vec3 deltaRotation = vector * realAngle;
//      data.rotation += deltaRotation;
//      data.orientation = glm::angleAxis(realAngle, vector) * data.orientation;
//      break;
//    }
//    case 0: {
//      float newPitch = m_pitch + realAngle;
//
//      if (!m_viewData.ignorePitchLimit) {
//        if (newPitch < -config::pitchLimit) {
//          break;
//        }
//
//        if (newPitch > config::pitchLimit) {
//          break;
//        }
//      }
//
//      m_pitch = newPitch;     // Case fall through is on purpose here
//    }
//    default: {
//      glm::vec3 deltaRotation = vector * realAngle;
//      data.rotation += deltaRotation;
//      data.orientation *= glm::angleAxis(realAngle, vector);
//      break;
//    }
//  }
//
//  switch (m_viewData.anchorFocusPoint) {
//    case true: {
//      // code for when the camera should be rotated around its focus point
//      break;
//    }
//    case false: {
//      data.forwardVector = glm::rotate(data.orientation, data.absoluteForwardVector);
//    }
//  }
//
//  data.wasUpdated = true;
//  updateAttachments();
//}

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

float ACamera::getFOV() noexcept {
  return m_viewData.perspectiveData.x;
}

void ACamera::setAspectRatio(float ratio) noexcept {
  if (m_projectionType == ECameraProjection::Orthogtaphic) return;

  m_viewData.perspectiveData.y = ratio;
  m_projection = glm::perspective(
      glm::radians(m_viewData.perspectiveData.x), m_viewData.perspectiveData.y,
      m_viewData.perspectiveData.z, m_viewData.perspectiveData.w);
}

float ACamera::getAspectRatio() noexcept {
  return (m_projectionType == ECameraProjection::Perspective) ? m_viewData.perspectiveData.y : 1.0f;
}

void ACamera::setViewBufferIndex(const uint32_t newIndex) {
  m_viewBufferIndex = newIndex;
}

const uint32_t ACamera::getViewBufferIndex() { return m_viewBufferIndex; }

bool ACamera::isBoundingBoxInFrustum(const WPrimitive* pPrimitive, const glm::mat4& projectionViewMatrix, const glm::mat4& modelMatrix) {
  const glm::mat4 MVPMatrix = projectionViewMatrix * modelMatrix;

  for (int32_t i = 0; i < 8; ++i) {
    glm::vec4 boxCorner = MVPMatrix * pPrimitive->extent.boxCorners[i];

    if (boxCorner.w == 0.0f) {
      return true;
    }

    if ((boxCorner.x > -boxCorner.w && boxCorner.x < boxCorner.w)
          || (boxCorner.y > -boxCorner.w && boxCorner.y < boxCorner.w)
          || (boxCorner.z > 0.0f && boxCorner.z < boxCorner.w)) {
      return true;
    }
  }

  return false;
}