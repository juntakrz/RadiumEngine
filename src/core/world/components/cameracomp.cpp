#include "pch.h"
#include "core/world/actors/base.h"
#include "core/world/components/transformcomp.h"
#include "core/world/components/cameracomp.h"

WCameraComponent::WCameraComponent(ABase* pActor) {
  typeId = EComponentType::Camera;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();

  // Subscribe to appropriate events
  pEvents->addDelegate<TransformUpdateComponentEvent>(this, &WCameraComponent::handleTransformUpdateEvent);
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

}

const glm::vec3& WCameraComponent::getTranslation() {
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

const glm::mat4& WCameraComponent::getView() {
  return data.view;
}

const glm::mat4& WCameraComponent::getProjection() {
  return data.projection;
}

void WCameraComponent::setIgnorePitchLimit(const bool newValue) {
  data.isIgnoringPitchLimit = newValue;
}

bool WCameraComponent::getIsIgnoringPitchLimit() {
  return data.isIgnoringPitchLimit;
}

void WCameraComponent::setAspectRatio(float newValue) noexcept {
  data.aspectRatio = newValue;

  data.projectionRequiresUpdate = true;
}

float WCameraComponent::getAspectRatio() noexcept {
  return data.aspectRatio;
}

void WCameraComponent::setViewBufferIndex(const uint32_t newIndex) {
  data.bufferViewIndex = newIndex;
}

uint32_t WCameraComponent::getViewBufferIndex() {
  return data.bufferViewIndex;
}

void WCameraComponent::update() {
  if (data.projectionRequiresUpdate) {
    switch (data.projectionMode) {
      case ECameraProjection::Perspective: {
        data.projection = glm::perspective(data.FOV, data.aspectRatio, data.nearPlane, data.viewDistance);
        break;
      }

      case ECameraProjection::Orthogtaphic: {
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

  const TransformUpdateComponentEvent& componentEvent = static_cast<const TransformUpdateComponentEvent&>(newEvent);

  // Determine if this event was sent by this component's owner or another actor entirely
  (componentEvent.pEventOwner == pOwner)
    ? data.ownerTranslation = componentEvent.translation
    : data.focusTranslation = componentEvent.translation;

  data.viewRequiresUpdate = true;
}
