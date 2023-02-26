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

void ACamera::rotate(const glm::vec3& delta) noexcept {
  m_transformationData.rotation =
      m_transformationData.rotation *
      (glm::quat(glm::mod(delta, math::twoPI)) * m_rotationModifier *
       core::time.getDeltaTime());

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
  m_transformationData.rotation *= glm::angleAxis(
      angle * m_rotationModifier * core::time.getDeltaTime(), vector);

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

void ACamera::rotate(const glm::quat& delta) noexcept {
  //glm::quat frameDelta = delta;
  //frameDelta.w *= m_rotationModifier * core::time.getDeltaTime();
  m_transformationData.rotation *=
      delta * m_rotationModifier * core::time.getDeltaTime();

  switch (m_viewData.bAnchorFocusPoint) {
    case true: {
      // code for when the camera should be rotated around its focus point
      break;
    }
    case false: {
      m_viewData.focusPoint = glm::rotate(m_transformationData.rotation,
                                          m_transformationData.frontVector);
      RE_LOG(Log, "Location: %.2f, %.2f, %.2f, Focus: %.2f, %.2f, %.2f",
             m_transformationData.translation.x,
             m_transformationData.translation.y,
             m_transformationData.translation.z, m_viewData.focusPoint.x,
             m_viewData.focusPoint.y, m_viewData.focusPoint.z);
    }
  }
}