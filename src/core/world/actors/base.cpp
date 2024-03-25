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

glm::mat4& ABase::getModelTransformationMatrix() noexcept {
  return getComponent<WTransformComponent>()->data.getModelTransformationMatrix();
}

void ABase::setTranslation(float x, float y, float z) noexcept {
  auto& data = getComponent<WTransformComponent>()->data;

  data.translation.x = x;
  data.translation.y = y;
  data.translation.z = z;
  data.wasUpdated = true;
}

void ABase::setTranslation(const glm::vec3& pos) noexcept {
  auto& data = getComponent<WTransformComponent>()->data;

  data.translation = pos;
  data.wasUpdated = true;
}

glm::vec3& ABase::getTranslation() noexcept { return getComponent<WTransformComponent>()->data.translation; }

void ABase::translate(const glm::vec3& delta) noexcept {
  auto& data = getComponent<WTransformComponent>()->data;

  data.translation += delta * m_translationModifier * core::time.getDeltaTime();  
  data.wasUpdated = true;
}

void ABase::setRotation(const glm::vec3& newRotation, const bool inRadians) noexcept {
  auto& data = getComponent<WTransformComponent>()->data;

  data.rotation = (inRadians) ? newRotation : glm::radians(newRotation);
  data.orientation = glm::quat(data.rotation);
  data.wasUpdated = true;
}

const glm::vec3& ABase::getRotation() noexcept {
  return getComponent<WTransformComponent>()->data.rotation;
}

const glm::quat& ABase::getOrientation() noexcept {
  return getComponent<WTransformComponent>()->data.orientation;
}

void ABase::rotate(const glm::vec3& delta, const bool ignoreFrameTime) noexcept {
  auto& data = getComponent<WTransformComponent>()->data;
  const float modifier = (ignoreFrameTime) ? 1.0f : m_rotationModifier * core::time.getDeltaTime();

  data.rotation += delta;
  data.orientation *= glm::quat(delta);
  math::wrapAnglesGLM(data.rotation);
  data.wasUpdated = true;
}

void ABase::setScale(const glm::vec3& scale) noexcept {
  auto& data = getComponent<WTransformComponent>()->data;

  data.scale = scale;
  data.wasUpdated = true;
}

void ABase::setScale(float scale) noexcept {
  auto& data = getComponent<WTransformComponent>()->data;

  data.scale.x = scale;
  data.scale.y = scale;
  data.scale.z = scale;
  data.wasUpdated = true;
}

const glm::vec3& ABase::getScale() noexcept { return getComponent<WTransformComponent>()->data.scale; }

void ABase::scale(const glm::vec3& delta) noexcept {
  auto& data = getComponent<WTransformComponent>()->data;

  data.scale *= delta * m_scalingModifier * core::time.getDeltaTime();
  data.wasUpdated = true;
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
  auto& data = getComponent<WTransformComponent>()->data;

  data.forwardVector = newVector;
}

glm::vec3& ABase::getForwardVector() {
  return getComponent<WTransformComponent>()->data.forwardVector;
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
  auto& data = getComponent<WTransformComponent>()->data;
  bool wasUpdated = data.wasUpdated;

  if (clearStatus) data.wasUpdated = false;

  return wasUpdated;
}

void ABase::drawComponentUIElements() {
  for (auto& it : m_pComponents) {
    it.second->showUIElement();
  }
}