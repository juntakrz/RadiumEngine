#include "pch.h"
#include "core/core.h"
#include "core/managers/time.h"
#include "core/world/actors/base.h"
#include "core/world/actors/camera.h"
#include "util/math.h"
#include "util/util.h"

void ABase::copyVec3ToMatrix(const float* vec3, float* matrixColumn) noexcept {
  __m128 srcVector = _mm_set_ps(0.0f, vec3[2], vec3[1], vec3[0]);
  __m128 dstColumn = _mm_loadu_ps(matrixColumn);
  dstColumn = _mm_blend_ps(dstColumn, srcVector, 0b0111);
  _mm_storeu_ps(matrixColumn, dstColumn);
}

void ABase::updateAttachments() {
  for (auto& pAttachment : m_pAttachments) {
    if (pAttachment.attachTranslation) {
      pAttachment.pAttached->setForwardVector(pAttachment.vector);
      pAttachment.pAttached->setLocation(
          pAttachment.attachToForwardVector
              ? this->getLocation() + this->getForwardVector() -
                    pAttachment.vector
              : this->getLocation() - pAttachment.vector);
    }
  }
}

glm::mat4& ABase::getRootTransformationMatrix() noexcept {
  if (m_transformationData.wasUpdated) {
    // Scale Rotation Translation (SRT) order

    // rotate and scale translated matrix
    m_transformationMatrix = glm::scale(m_transformationData.scaling) *
                             glm::mat4_cast(m_transformationData.rotation);

    // use SIMD to add translation data without creating the new matrix
    copyVec3ToMatrix(&m_transformationData.translation.x,
                     &m_transformationMatrix[3][0]);

    updateAttachments();

    m_transformationData.wasUpdated = false;
  }
  return m_transformationMatrix;
}

ABase::TransformationData& ABase::getTransformData() noexcept {
  return m_transformationData;
}

void ABase::setLocation(float x, float y, float z) noexcept {
  m_transformationData.translation.x = x;
  m_transformationData.translation.y = y;
  m_transformationData.translation.z = z;

  m_transformationData.initial.translation = m_transformationData.translation;

  m_transformationData.wasUpdated = true;
}

void ABase::setLocation(const glm::vec3& pos) noexcept {
  m_transformationData.translation = pos;
  m_transformationData.initial.translation = m_transformationData.translation;

  m_transformationData.wasUpdated = true;
}

glm::vec3& ABase::getLocation() noexcept { return m_transformationData.translation; }

void ABase::translate(const glm::vec3& delta) noexcept {
  m_transformationData.translation +=
      delta * m_translationModifier * core::time.getDeltaTime();
      
  m_transformationData.wasUpdated = true;
}

void ABase::setRotation(const glm::vec3& delta) noexcept {
  m_transformationData.rotation = glm::quat(delta);
  m_transformationData.initial.rotation = m_transformationData.rotation;

  m_transformationData.wasUpdated = true;
}

void ABase::setRotation(const glm::vec3& vector, float angle) noexcept {
  m_transformationData.rotation = glm::angleAxis(angle, vector);
  m_transformationData.initial.rotation = m_transformationData.rotation;

  m_transformationData.wasUpdated = true;
}

void ABase::setRotation(const glm::quat& newRotation) noexcept {
  m_transformationData.rotation = newRotation;
  m_transformationData.initial.rotation = m_transformationData.rotation;

  m_transformationData.wasUpdated = true;
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

  m_transformationData.wasUpdated = true;
}

void ABase::rotate(const glm::vec3& vector, float angle) noexcept {
  m_transformationData.rotation *= glm::quat(angle, vector);

  m_transformationData.wasUpdated = true;
}

void ABase::rotate(const glm::quat& delta) noexcept {
  m_transformationData.rotation *= delta;

  m_transformationData.wasUpdated = true;
}

void ABase::setScale(const glm::vec3& scale) noexcept {
  m_transformationData.scaling = scale;
  m_transformationData.initial.scaling = m_transformationData.scaling;

  m_transformationData.wasUpdated = true;
}

void ABase::setScale(float scale) noexcept {
  m_transformationData.scaling.x = scale;
  m_transformationData.scaling.y = scale;
  m_transformationData.scaling.z = scale;
  m_transformationData.initial.scaling = m_transformationData.scaling;

  m_transformationData.wasUpdated = true;
}

glm::vec3& ABase::getScale() noexcept { return m_transformationData.scaling; }

void ABase::scale(const glm::vec3& delta) noexcept {
  m_transformationData.scaling *= delta * m_scalingModifier * core::time.getDeltaTime();

  m_transformationData.wasUpdated = true;
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

void ABase::setForwardVector(const glm::vec3& newVector) {
  m_transformationData.forwardVector = newVector;
}

glm::vec3& ABase::getForwardVector() {
  return m_transformationData.forwardVector;
}

void ABase::setName(const char* name) { m_name = name; }

const char* ABase::getName() { return m_name.c_str(); }

const EActorType& ABase::getTypeId() { return m_typeId; }

void ABase::setVisibility(const bool isVisible) { m_isVisible = isVisible; }

const bool ABase::isVisible() { return m_isVisible; }

void ABase::attachTo(ABase* pTarget, const bool toTranslation,
                     const bool toRotation, const bool toForwardVector) {
  if (!pTarget) {
    RE_LOG(Error, "Failed to attach '%s' to target. No target was provided.",
           m_name.c_str());
    return;
  }

  if (!toTranslation && !toRotation) {
    RE_LOG(Error,
           "Unable to attach '%s' to '%s' because no valid attachment "
           "parameters were provided.",
           m_name.c_str(), pTarget->m_name.c_str());
    return;
  }

  pTarget->m_pAttachments.emplace_back();

  WAttachmentInfo& info = pTarget->m_pAttachments.back();
  info.pAttached = this;
  info.attachTranslation = toTranslation;
  info.attachRotation = toRotation;
  info.attachToForwardVector = toForwardVector;

  // get attachment vector that will keep attached object relative
  switch (info.attachToForwardVector) {
    case true: {
      info.vector = pTarget->getForwardVector() - this->getLocation();
      break;
    }
    case false: {
      info.vector = pTarget->getLocation() - this->getLocation();
    }
  }

  pTarget->updateAttachments();
}