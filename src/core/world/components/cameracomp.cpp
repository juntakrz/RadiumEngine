#include "pch.h"
#include "core/world/actors/base.h"
#include "core/world/components/transformcomp.h"
#include "core/world/components/cameracomp.h"

WCameraComponent::WCameraComponent(ABase* pActor) {
  typeId = EComponentType::Camera;
  pOwner = pActor;

  // This component requires an actor to have the transform component
  if (!(pTransform = pOwner->getComponent<WTransformComponent>())) {
    pTransform = pOwner->addComponent<WTransformComponent>();
  }
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

const glm::mat4& WCameraComponent::getView() {
  return data.view;
}

const glm::mat4& WCameraComponent::getProjection() {
  return data.projection;
}

void WCameraComponent::update() {
  if (data.projectionRequiresUpdate) {
    switch (data.projectionMode) {
      case ECameraProjection::Perspective: {
        data.projection = glm::perspective(data.FOV, config::getAspectRatio(), data.nearPlane, data.viewDistance);
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
        data.view = glm::lookAt(
          data.translation + pTransform->getTranslation(),
          data.translation + pTransform->getTranslation() + data.forwardVector,
          data.upVector);
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
