#include "pch.h"
#include "core/core.h"
#include "core/managers/time.h"
#include "core/world/actors/base.h"
#include "core/world/actors/camera.h"
#include "util/math.h"
#include "util/util.h"

glm::mat4& ABase::getTransformation() noexcept { return m_transformationMatrix; }

ABase::TransformationData& ABase::getTransformData() noexcept {
  return m_transformationData;
}

void ABase::setLocation(float x, float y, float z) noexcept {
  m_transformationData.translation.x = x;
  m_transformationData.translation.y = y;
  m_transformationData.translation.z = z;

  m_transformationData.initial.translation = m_transformationData.translation;
}

void ABase::setLocation(const glm::vec3& pos) noexcept {
  m_transformationData.translation = pos;
  m_transformationData.initial.translation = m_transformationData.translation;
}

glm::vec3& ABase::getLocation() noexcept { return m_transformationData.translation; }

void ABase::translate(const glm::vec3& delta) noexcept {
  m_transformationData.translation +=
      delta * m_translationModifier * core::time.getDeltaTime();
}

void ABase::setRotation(const glm::vec3& delta) noexcept {
  m_transformationData.rotation = glm::quat(delta);
  m_transformationData.initial.rotation = m_transformationData.rotation;
}

void ABase::setRotation(const glm::vec3& vector, float angle) noexcept {
  m_transformationData.rotation = glm::quat(angle, vector);
  m_transformationData.initial.rotation = m_transformationData.rotation;
}

void ABase::setRotation(const glm::quat& newRotation) noexcept {
  m_transformationData.rotation = newRotation;
  m_transformationData.initial.rotation = m_transformationData.rotation;
}

glm::quat& ABase::getRotation() noexcept {
  return m_transformationData.rotation;
}

glm::vec3 ABase::getRotationAngles() noexcept {
  return glm::eulerAngles(m_transformationData.rotation) * (180.0f / math::PI);
}

void ABase::rotate(const glm::vec3& delta) noexcept {
  m_transformationData.rotation =
      glm::mod(delta * m_rotationModifier * core::time.getDeltaTime(),
               math::twoPI);
}

void ABase::rotate(const glm::vec3& vector, float angle) noexcept {
  m_transformationData.rotation *= glm::quat(angle, vector);
}

void ABase::rotate(const glm::quat& delta) noexcept {
  m_transformationData.rotation *= delta;
}

void ABase::setScale(const glm::vec3& scale) noexcept {
  m_transformationData.scaling = scale;
  m_transformationData.initial.scaling = m_transformationData.scaling;
}

glm::vec3& ABase::getScale() noexcept { return m_transformationData.scaling; }

void ABase::scale(const glm::vec3& delta) noexcept {
  m_transformationData.scaling *= delta * m_scalingModifier * core::time.getDeltaTime();
}

void ABase::setTranslationModifier(float newModifier) {
  m_translationModifier = newModifier;
}

void ABase::setRotationModifier(float newModifier) {
  m_rotationModifier = newModifier;
}

void ABase::setScalingModifier(float newModifier) {
  m_scalingModifier = newModifier;
}

const EActorType& ABase::typeId() { return m_typeId; }