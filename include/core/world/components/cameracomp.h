#pragma once

#include "component.h"

struct WCameraComponent : public WComponent {
  struct {
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    float FOV = 75.0f;
    float orthoFOV = 1.0f;
    float nearPlane = 0.0001f;
    float viewDistance = 1000.0f;

    glm::vec3 translation = glm::vec3(0.0f);      // Translation from origin
    glm::vec3 rotation = glm::vec3(1.0f);
    glm::quat orientation = glm::quat(rotation);

    glm::vec3 upVector = { 0.0f, 1.0f, 0.0f };
    glm::vec3 forwardVector = { 0.0f, 0.0f, 1.0f };
    glm::vec3 absoluteForwardVector = { 0.0f, 0.0f, 1.0f };

    ECameraView viewMode = ECameraView::LookAt;
    ECameraProjection projectionMode = ECameraProjection::Perspective;

    bool projectionRequiresUpdate = true;
    bool viewRequiresUpdate = true;
  } data;

  struct WTransformComponent* pTransform = nullptr;

  WCameraComponent(ABase* pActor);

  void setProjectionMode(ECameraProjection newMode);
  void setFOV(float newFOV);
  void setOrthoFOV(float newFOV);
  void setViewDistance(float newVideDistance);

  const ECameraProjection getProjectionMode();
  const float getFOV();
  const float getOrthoFOV();
  const float getViewDistance();

  const glm::mat4& getView();
  const glm::mat4& getProjection();

  void update() override;

  void drawComponentUI() override;
};