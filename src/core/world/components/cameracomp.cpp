#include "pch.h"
#include "util/math.h"
#include "core/world/actors/base.h"
#include "core/world/components/transformcomp.h"
#include "core/world/components/cameracomp.h"

WCameraComponent::WCameraComponent(ABase* pActor) {
  typeId = EComponentType::Camera;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();

  // Subscribe to appropriate events
  pEvents->addDelegate<TransformUpdateComponentEvent>(this, &WCameraComponent::handleTransformUpdateEvent);
  pEvents->addDelegate<ControllerRotationComponentEvent>(this, &WCameraComponent::handleControllerRotation);
}

void WCameraComponent::setTranslationOffset(float x, float y, float z, bool isDelta) {
  data.translation.x = (isDelta) ? data.translation.x + x  : x;
  data.translation.y = (isDelta) ? data.translation.y + y  : y;
  data.translation.z = (isDelta) ? data.translation.z + z  : z;

  data.viewRequiresUpdate = true;
}

void WCameraComponent::setTranslationOffset(const glm::vec3& newTranslation, bool isDelta) {
  data.translation = (isDelta) ? data.translation + newTranslation : newTranslation;

  data.viewRequiresUpdate = true;
}

void WCameraComponent::setRotation(const glm::vec3& newRotation, bool isInRadians, bool isDelta) {
  glm::vec3 rotation = (isInRadians) ? newRotation : glm::radians(newRotation);

  switch (isDelta) {
    case true: {
      bool isYaw = rotation.y != 0.0f ? true : false;

      float newPitch = rotation.x + data.rotation.x;
      if (!data.isIgnoringPitchLimit && (newPitch < -config::pitchLimit || newPitch > config::pitchLimit)) {
        break;
      }

      data.rotation += rotation;
      math::wrapAnglesGLM(data.rotation);
      data.orientation = (isYaw)
        ? glm::quat(rotation) * data.orientation
        : data.orientation * glm::quat(rotation);
      break;
    }

    case false: {
      float newPitch = data.isIgnoringPitchLimit ? rotation.x :
        rotation.x < -config::pitchLimit ? -config::pitchLimit
        : rotation.x > config::pitchLimit ? config::pitchLimit
        : rotation.x;

      data.rotation.x = newPitch;
      data.rotation.y = rotation.y;
      data.rotation.z = rotation.z;
      math::wrapAnglesGLM(data.rotation);
      data.orientation = glm::quat(data.rotation);

      break;
    }
  }

  data.forwardVector = glm::rotate(data.orientation, data.absoluteForwardVector);

  data.viewRequiresUpdate = true;
}

const glm::vec3 WCameraComponent::getTranslation() {
  return data.ownerTranslation + data.translation;
}

const glm::vec3& WCameraComponent::getTranslationOffset() {
  return data.translation;
}

const glm::vec3& WCameraComponent::getRotation() {
  return data.rotation;
}

void WCameraComponent::setProjectionMode(ECameraProjection newMode) {
  data.projectionMode = newMode;
  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setFOV(float newFOV) {
  data.FOV = newFOV;
  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setOrthoFOV(float newFOV) {
  data.orthoFOV = newFOV;
  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setViewDistance(float newVideDistance) {
  data.viewDistance = newVideDistance;
  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setAspectRatio(float newValue) noexcept {
  data.aspectRatio = newValue;

  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setCameraParameters(ECameraProjection newMode, float newFOV, float aspectRatio, float newViewDistance) {
  (newMode == ECameraProjection::Perspective) ? data.FOV = newFOV : data.orthoFOV = newFOV;

  if (newMode ==ECameraProjection::Perspective) {
    data.aspectRatio = aspectRatio;
  }

  data.projectionMode = newMode;
  data.viewDistance = newViewDistance;

  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setFocus(ECameraFocusMode newFocusMode, ABase* pTargetActor) {
  if ((data.focusMode = newFocusMode) == ECameraFocusMode::None) return;

  if (!pTargetActor || !(pTargetActor->getComponent<WTransformComponent>())) {
    RE_LOG(Error, "Focus mode has been selected for camera component belonging to the"
      "actor '%s', but no target was provided or the target does not have a transform component.");
    data.focusMode = ECameraFocusMode::None;
    return;
  }

  // Add delegate to track target actor's transformation changes
  pTargetActor->getEventSystem().addDelegate<WTransformComponent>(this, &WCameraComponent::handleTransformUpdateEvent);
}

const ECameraProjection WCameraComponent::getProjectionMode() {
  return data.projectionMode;
}

const float WCameraComponent::getFOV() {
  return data.FOV;
}

const float WCameraComponent::getOrthoFOV() {
  return data.orthoFOV;
}

const float WCameraComponent::getViewDistance() {
  return data.viewDistance;
}

const glm::vec2 WCameraComponent::getClipPlanes() {
  return { data.nearPlane, data.viewDistance };
}

float WCameraComponent::getAspectRatio() noexcept {
  return data.aspectRatio;
}

const glm::mat4& WCameraComponent::getView() {
  if (data.viewRequiresUpdate) {
    update();
  }

  return data.view;
}

const glm::mat4& WCameraComponent::getProjection() {
  if (data.projectionRequiresUpdate) {
    update();
  }

  return data.projection;
}

void WCameraComponent::setForwardVector(const glm::vec3& newVector) {
  data.forwardVector = newVector;
  data.viewRequiresUpdate = true;
}

void WCameraComponent::setAbsoluteForwardVector(const glm::vec3& newVector) {
  data.absoluteForwardVector = newVector;
  data.viewRequiresUpdate = true;
}

void WCameraComponent::setUpVector(const glm::vec3& newVector) {
  data.upVector = newVector;
  data.viewRequiresUpdate = true;
}

const glm::vec3& WCameraComponent::getForwardVector() {
  return data.forwardVector;
}

const glm::vec3& WCameraComponent::getAbsoluteForwardVector() {
  return data.absoluteForwardVector;
}

const glm::vec3& WCameraComponent::getUpVector() {
  return data.upVector;
}

void WCameraComponent::setIgnorePitchLimit(const bool newValue) {
  data.isIgnoringPitchLimit = newValue;
}

bool WCameraComponent::getIsIgnoringPitchLimit() {
  return data.isIgnoringPitchLimit;
}

void WCameraComponent::setViewBufferIndex(const uint32_t newIndex) {
  data.bufferViewIndex = newIndex;
}

uint32_t WCameraComponent::getViewBufferIndex() {
  return data.bufferViewIndex;
}

void WCameraComponent::onOwnerPossessed() {
  pEvents->addDelegate<ControllerRotationComponentEvent>(this, &WCameraComponent::handleControllerRotation);
}

void WCameraComponent::update() {
  if (data.projectionRequiresUpdate) {
    switch (data.projectionMode) {
      case ECameraProjection::Perspective: {
        data.projection = glm::perspective(glm::radians(data.FOV), data.aspectRatio, data.nearPlane, data.viewDistance);
        break;
      }

      case ECameraProjection::Orthographic: {
        data.projection = glm::ortho(-data.orthoFOV, data.orthoFOV, -data.orthoFOV, data.orthoFOV);
        break;
      }
    }

    data.projectionRequiresUpdate = false;
  }

  if (data.viewRequiresUpdate) {
    switch (data.viewMode) {
      case ECameraView::LookAt: {
        switch (data.focusMode) {
          case ECameraFocusMode::Translation: {
            data.view = glm::lookAt(
              data.translation + data.ownerTranslation,
              data.translation + data.ownerTranslation + data.focusTranslation,
              data.upVector);
            break;
          }

          default: {
            data.view = glm::lookAt(
              data.translation + data.ownerTranslation,
              data.translation + data.ownerTranslation + data.forwardVector,
              data.upVector);
            break;
          }
        }

        break;
      }
      case ECameraView::Free: {
        break;
      }
    }

    data.viewRequiresUpdate = false;
  }
}

void WCameraComponent::drawComponentUI() {
}

void WCameraComponent::handleTransformUpdateEvent(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(TransformUpdateComponentEvent)) {
    RE_LOG(Error, "Invalid component event type for '%s'.", pOwner->getName().c_str());
    return;
  }

  const TransformUpdateComponentEvent& componentEvent =
    static_cast<const TransformUpdateComponentEvent&>(newEvent);

  // Determine if this event was sent by this component's owner or another actor entirely
  (componentEvent.pEventOwner == pOwner)
    ? data.ownerTranslation = componentEvent.translation
    : data.focusTranslation = componentEvent.translation;

  data.viewRequiresUpdate = true;
}

void WCameraComponent::handleControllerRotation(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(ControllerRotationComponentEvent)
    || newEvent.pEventOwner != pOwner) return;

  const ControllerRotationComponentEvent componentEvent =
    static_cast<const ControllerRotationComponentEvent&>(newEvent);

  setRotation(componentEvent.controllerRotationDelta, true, true);
}
