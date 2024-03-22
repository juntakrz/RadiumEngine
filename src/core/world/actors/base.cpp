#include "pch.h"
#include "core/core.h"
#include "core/managers/ref.h"
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
      pAttachment.pAttached->setTranslation(
          pAttachment.attachToForwardVector
              ? this->getTranslation() + this->getForwardVector() -
                    pAttachment.vector
              : this->getTranslation() - pAttachment.vector);
    }
  }
}

glm::mat4& ABase::getRootTransformationMatrix() noexcept {
  if (m_transformationData.wasUpdated) {
    // Scale Rotation Translation (SRT) order

    // rotate and scale translated matrix
    m_transformationMatrix =
      glm::scale(m_transformationData.scaling) * glm::mat4_cast(m_transformationData.orientation);

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

void ABase::setTranslation(float x, float y, float z) noexcept {
  m_transformationData.translation.x = x;
  m_transformationData.translation.y = y;
  m_transformationData.translation.z = z;

  m_transformationData.wasUpdated = true;
}

void ABase::setTranslation(const glm::vec3& pos) noexcept {
  m_transformationData.translation = pos;

  m_transformationData.wasUpdated = true;
}

glm::vec3& ABase::getTranslation() noexcept { return m_transformationData.translation; }

void ABase::translate(const glm::vec3& delta) noexcept {
  m_transformationData.translation +=
      delta * m_translationModifier * core::time.getDeltaTime();
      
  m_transformationData.wasUpdated = true;
}

void ABase::setRotation(const glm::vec3& newRotation, const bool inRadians) noexcept {
  m_transformationData.rotation = (inRadians) ? newRotation : glm::radians(newRotation);
  m_transformationData.orientation = glm::quat(m_transformationData.rotation);

  m_transformationData.wasUpdated = true;
}

const glm::vec3& ABase::getRotation() noexcept {
  return m_transformationData.rotation;
}

const glm::quat& ABase::getOrientation() noexcept {
  return m_transformationData.orientation;
}

void ABase::rotate(const glm::vec3& delta, const bool ignoreFrameTime) noexcept {
  const float modifier = (ignoreFrameTime) ? 1.0f : m_rotationModifier * core::time.getDeltaTime();

  m_transformationData.rotation += delta;
  m_transformationData.orientation *= glm::quat(delta);
  //math::wrapAnglesGLM(m_transformationData.rotation);
  //m_transformationData.rotation = glm::mod(m_transformationData.rotation, math::twoPI);

  m_transformationData.wasUpdated = true;
}

void ABase::setScale(const glm::vec3& scale) noexcept {
  m_transformationData.scaling = scale;

  m_transformationData.wasUpdated = true;
}

void ABase::setScale(float scale) noexcept {
  m_transformationData.scaling.x = scale;
  m_transformationData.scaling.y = scale;
  m_transformationData.scaling.z = scale;

  m_transformationData.wasUpdated = true;
}

const glm::vec3& ABase::getScale() noexcept { return m_transformationData.scaling; }

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

void ABase::setName(const std::string& name) {
  m_previousName = m_name;
  m_name = name;

  core::ref.registerActor(this);
}

const std::string& ABase::getName() { return m_name; }

const std::string& ABase::getPreviousName() { return m_previousName; }

const EActorType& ABase::getTypeId() { return m_typeId; }

const uint32_t ABase::getUID() { return m_UID; }

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
      info.vector = pTarget->getForwardVector() - this->getTranslation();
      break;
    }
    case false: {
      info.vector = pTarget->getTranslation() - this->getTranslation();
    }
  }

  pTarget->updateAttachments();
}

bool ABase::wasUpdated(const bool clearStatus) {
  bool wasUpdated = m_transformationData.wasUpdated;

  if (clearStatus) m_transformationData.wasUpdated = false;

  return wasUpdated;
}
