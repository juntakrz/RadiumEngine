#include "pch.h"
#include "core/core.h"
#include "core/managers/ref.h"
#include "core/managers/time.h"
#include "core/world/actors/base.h"
#include "core/world/actors/camera.h"
#include "core/world/components/components.h"
#include "util/math.h"
#include "util/util.h"

ABase::ABase(const uint32_t UID) {
  m_UID = UID;

  // An actor always has transform component
  addComponent<WTransformComponent>();
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

void ABase::setForwardVector(const glm::vec3& newVector) {
  m_forwardVector = newVector;
}

const glm::vec3& ABase::getForwardVector() {
  return m_forwardVector;
}

const glm::vec3& ABase::getDefaultForwardVector() {
  return m_defaultForwardVector;
}

void ABase::setUpVector(const glm::vec3& newVector) {
  m_upVector = newVector;
}

const glm::vec3& ABase::getUpVector() {
  return m_upVector;
}

const glm::vec3& ABase::getDefaultUpVector() {
  return m_defaultUpVector;
}

void ABase::setControlMode(EActorControlMode newMode) {
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

EActorControlMode ABase::getControlMode() {
  return m_controlMode;
}

void ABase::onControllerMovement(const glm::vec3& vector, const bool isRotation) {
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

void ABase::onControlled(core::MPlayer* pController) {
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

void ABase::onFreed() {
  for (auto& componentType : m_pComponents) {
    for (auto& component : componentType.second) {
      component->onOwnerFreed();
    }
  }
}

core::MPlayer* ABase::getController() {
  return m_pController;
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

void ABase::attachTo(ABase* pTarget, EAttachmentMode newMode) {
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

void ABase::detach() {
  attachTo(nullptr, EAttachmentMode::None);
}

void ABase::updateComponents() {
  for (auto& componentType : m_pComponents) {
    for (auto& component : componentType.second) {
      component->update();
    }
  }
}

void ABase::drawComponentUIElements() {
  for (auto& componentType : m_pComponents) {
    for (auto& component : componentType.second) {
      component->drawComponentUI();
    }
  }
}