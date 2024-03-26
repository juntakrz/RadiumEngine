#include "pch.h"
#include "core/core.h"
#include "core/managers/ref.h"
#include "core/managers/time.h"
#include "core/world/actors/base.h"
#include "core/world/actors/camera.h"
#include "core/world/components/components.h"
#include "util/math.h"
#include "util/util.h"

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

const glm::mat4& ABase::getModelTransformationMatrix() noexcept {
  return getComponent<WTransformComponent>()->getModelTransformationMatrix();
}

void ABase::setTranslation(float x, float y, float z, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setTranslation(x, y, z, isDelta);
}

void ABase::setTranslation(const glm::vec3& newLocation, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setTranslation(newLocation, isDelta);
}

void ABase::setRotation(float x, float y, float z, bool isInRadians, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setRotation(x, y, z, isInRadians, isDelta);
}

void ABase::setRotation(const glm::vec3& newRotation, bool isInRadians, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setRotation(newRotation, isInRadians, isDelta);
}

void ABase::setScale(const glm::vec3& scale, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setScale(scale, isDelta);
}

void ABase::setScale(float newScale, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setScale(newScale, isDelta);
}

void ABase::setForwardVector(const glm::vec3& newVector) {
  getComponent<WTransformComponent>()->setForwardVector(newVector);
}

void ABase::setAbsoluteForwardVector(const glm::vec3& newVector) {
  getComponent<WTransformComponent>()->setAbsoluteForwardVector(newVector);
}

const glm::vec3& ABase::getTranslation() noexcept {
  return getComponent<WTransformComponent>()->getTranslation();
}

const glm::vec3& ABase::getRotation() noexcept {
  return getComponent<WTransformComponent>()->getRotation();
}

const glm::quat& ABase::getOrientation() noexcept {
  return getComponent<WTransformComponent>()->getOrientation();
}

const glm::vec3& ABase::getScale() noexcept {
  return getComponent<WTransformComponent>()->getScale();
}

void ABase::setTranslationModifier(float newModifier) {
  getComponent<WTransformComponent>()->setTranslationDeltaModifier(newModifier);
}

void ABase::setRotationModifier(float newModifier) {
  getComponent<WTransformComponent>()->setRotationDeltaModifier(newModifier);
}

void ABase::setScalingModifier(float newModifier) {
  getComponent<WTransformComponent>()->setScaleDeltaModifier(newModifier);
}

const glm::vec3& ABase::getForwardVector() {
  return getComponent<WTransformComponent>()->getForwardVector();
}

const glm::vec3& ABase::getAbsoluteForwardVector() {
  return getComponent<WTransformComponent>()->getAbsoluteForwardVector();
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

void ABase::updateComponents() {
  for (const auto& it : m_pComponents) {
    it.second->update();
  }
}

void ABase::drawComponentUIElements() {
  for (auto& it : m_pComponents) {
    it.second->drawComponentUI();
  }
}