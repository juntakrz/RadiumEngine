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
}

void WCameraComponent::setTranslationOffset(float x, float y, float z, bool isDelta) {
  data.translationOffset.x = (isDelta) ? data.translationOffset.x + x  : x;
  data.translationOffset.y = (isDelta) ? data.translationOffset.y + y  : y;
  data.translationOffset.z = (isDelta) ? data.translationOffset.z + z  : z;

  data.viewRequiresUpdate = true;
}

void WCameraComponent::setTranslationOffset(const glm::vec3& newTranslation, bool isDelta) {
  data.translationOffset = (isDelta) ? data.translationOffset + newTranslation : newTranslation;

  data.viewRequiresUpdate = true;
}

const glm::vec3 WCameraComponent::getWorldTranslation() {
  return data.ownerTranslation + data.translationOffset;
}

const glm::vec3& WCameraComponent::getTranslationOffset() {
  return data.translationOffset;
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
    switch (attachmentMode) {
      case EAttachmentMode::Translation: {
        data.view = glm::lookAt(
          getWorldTranslation(), getWorldTranslation() - data.focusVector,
          pOwner->getUpVector());
        break;
      }

      default: {
        data.view = glm::lookAt(
          getWorldTranslation(), getWorldTranslation() + pOwner->getForwardVector(),
          pOwner->getUpVector());
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
    invalidComponentErrorMessage();
    return;
  }

  const TransformUpdateComponentEvent& componentEvent =
    static_cast<const TransformUpdateComponentEvent&>(newEvent);

  data.ownerTranslation = componentEvent.translation;
  data.ownerOrientation = componentEvent.orientation;
  data.focusVector = componentEvent.attachmentVector;

  data.viewRequiresUpdate = true;
}