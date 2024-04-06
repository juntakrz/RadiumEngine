#include "pch.h"
#include "core/core.h"
#include "core/managers/gui.h"
#include "core/managers/scene.h"
#include "core/managers/time.h"
#include "core/world/actor.h"
#include "core/world/components/components.h"
#include "util/math.h"
#include "util/util.h"

WActor::WActor(const uint32_t UID) {
  m_UID = UID;

  // An actor always has transform component
  addComponent<WTransformComponent>();
}

const glm::mat4& WActor::getModelTransformationMatrix() noexcept {
  return getComponent<WTransformComponent>()->getModelTransformationMatrix();
}

void WActor::setTranslation(float x, float y, float z, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setTranslation(x, y, z, isDelta);
}

void WActor::setTranslation(const glm::vec3& newLocation, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setTranslation(newLocation, isDelta);
}

void WActor::setRotation(float x, float y, float z, bool isInRadians, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setRotation(x, y, z, isInRadians, isDelta);
}

void WActor::setRotation(const glm::vec3& newRotation, bool isInRadians, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setRotation(newRotation, isInRadians, isDelta);
}

void WActor::setScale(const glm::vec3& scale, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setScale(scale, isDelta);
}

void WActor::setScale(float newScale, bool isDelta) noexcept {
  getComponent<WTransformComponent>()->setScale(newScale, isDelta);
}

const glm::vec3& WActor::getTranslation() noexcept {
  return getComponent<WTransformComponent>()->getTranslation();
}

const glm::vec3& WActor::getRotation() noexcept {
  return getComponent<WTransformComponent>()->getRotation();
}

const glm::quat& WActor::getOrientation() noexcept {
  return getComponent<WTransformComponent>()->getOrientation();
}

const glm::vec3& WActor::getScale() noexcept {
  return getComponent<WTransformComponent>()->getScale();
}

void WActor::setTranslationModifier(float newModifier) {
  getComponent<WTransformComponent>()->setTranslationDeltaModifier(newModifier);
}

void WActor::setRotationModifier(float newModifier) {
  getComponent<WTransformComponent>()->setRotationDeltaModifier(newModifier);
}

void WActor::setScalingModifier(float newModifier) {
  getComponent<WTransformComponent>()->setScaleDeltaModifier(newModifier);
}

void WActor::setForwardVector(const glm::vec3& newVector) {
  m_forwardVector = newVector;
}

const glm::vec3& WActor::getForwardVector() {
  return m_forwardVector;
}

const glm::vec3& WActor::getDefaultForwardVector() {
  return m_defaultForwardVector;
}

void WActor::setUpVector(const glm::vec3& newVector) {
  m_upVector = newVector;
}

const glm::vec3& WActor::getUpVector() {
  return m_upVector;
}

const glm::vec3& WActor::getDefaultUpVector() {
  return m_defaultUpVector;
}

void WActor::setControlMode(EActorControlMode newMode) {
  m_controlMode = newMode;

  if (m_controlMode == EActorControlMode::FirstPerson) {
    setUpVector(glm::vec3(0.0f, 1.0f, 0.0f));
  }

  for (auto& componentType : m_pComponents) {
    for (auto& component : componentType.second) {
      component->onOwnerUpdated();
    }
  }
}

EActorControlMode WActor::getControlMode() {
  return m_controlMode;
}

void WActor::onControllerMovement(const glm::vec3& vector, const bool isRotation) {
  switch (isRotation) {
    case true: {
      ControllerRotationComponentEvent newEvent;
      newEvent.pEventOwner = this;
      newEvent.controllerRotationDelta = vector;

      m_eventSystem.sendEvent<ControllerRotationComponentEvent>(newEvent);
      break;
    }

    case false: {
      ControllerTranslationComponentEvent newEvent;
      newEvent.pEventOwner = this;
      newEvent.controllerTranslationDelta = vector;

      m_eventSystem.sendEvent<ControllerTranslationComponentEvent>(newEvent);
      break;
    }
  }
}

void WActor::onControlled(core::MPlayer* pController) {
  if (!pController || m_pController == pController) return;
  
  onFreed();

  m_pController = pController;
  m_controlMode = m_pController->getProperties().controlMode;

  for (auto& componentType : m_pComponents) {
    for (auto& component : componentType.second) {
      component->onOwnerControlled();
    }
  }
}

void WActor::onFreed() {
  for (auto& componentType : m_pComponents) {
    for (auto& component : componentType.second) {
      component->onOwnerFreed();
    }
  }
}

core::MPlayer* WActor::getController() {
  return m_pController;
}

void WActor::setName(const std::string& name) {
  m_previousName = m_name;
  m_name = name;

  core::scene.registerActor(this);
}

const std::string& WActor::getName() { return m_name; }

const std::string& WActor::getPreviousName() { return m_previousName; }

const EActorType& WActor::getTypeId() { return m_typeId; }

const uint32_t WActor::getUID() { return m_UID; }

void WActor::setVisibility(const bool isVisible) { m_isVisible = isVisible; }

const bool WActor::isVisible() { return m_isVisible; }

void WActor::attachTo(WActor* pTarget, EAttachmentMode newMode) {
  if (!pTarget) {
    RE_LOG(Error, "Failed to attach '%s' to target, nullptr was received.", getName().c_str());
    return;
  }

  attachmentInfo.attachmentMode = newMode;

  switch (newMode) {
    case EAttachmentMode::None: {
      attachmentInfo.pTarget = nullptr;
      break;
    }
    case EAttachmentMode::Translation: {
      attachmentInfo.pTarget = pTarget;
      break;
    }
    case EAttachmentMode::TranslationAndRotation: {
      attachmentInfo.pTarget = pTarget;
      break;
    }
  }

  for (auto& componentType : m_pComponents) {
    for (auto& component : componentType.second) {
      component->onAttachmentModeChanged(attachmentInfo.pTarget, attachmentInfo.attachmentMode);
    }
  }
}

void WActor::detach() {
  attachTo(nullptr, EAttachmentMode::None);
}

void WActor::updateComponents() {
  for (auto& componentType : m_pComponents) {
    for (auto& component : componentType.second) {
      component->update();
    }
  }
}

void WActor::forceUpdateTransform() {
  getComponent<WTransformComponent>()->forceUpdateTransform();
}

void WActor::drawComponentUIElements() {
  // Always draw transform component's editor UI first
  getComponent<WTransformComponent>()->drawComponentUI(0);

  for (auto& componentType : m_pComponents) {
    if (core::gui.isSkippingFrame()) return;
    uint32_t index = 0;

    for (auto& component : componentType.second) {
      if (core::gui.isSkippingFrame()) return;

      if (component->typeId != EComponentType::Transform) {
        component->drawComponentUI(index);
        ++index;
      }
    }
  }
}