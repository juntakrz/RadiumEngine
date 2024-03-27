#pragma once

#include "component.h"

struct WCameraComponent : public WComponent {
  struct {
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    glm::vec3 translation = glm::vec3(0.0f);      // Translation from origin
    glm::vec3 rotation = glm::vec3(1.0f);
    glm::quat orientation = glm::quat(rotation);

    glm::vec3 upVector = { 0.0f, 1.0f, 0.0f };
    glm::vec3 forwardVector = { 0.0f, 0.0f, 1.0f };
    glm::vec3 absoluteForwardVector = { 0.0f, 0.0f, 1.0f };

    float FOV = 75.0f;
    float orthoFOV = 1.0f;
    float nearPlane = 0.0001f;
    float viewDistance = 1000.0f;
    float aspectRatio = 1.0f;

    bool isIgnoringPitchLimit = false;

    ECameraFocusMode focusMode = ECameraFocusMode::None;
    ECameraView viewMode = ECameraView::LookAt;
    ECameraProjection projectionMode = ECameraProjection::Perspective;

    bool projectionRequiresUpdate = true;
    bool viewRequiresUpdate = true;

    uint32_t bufferViewIndex = -1;

    // Event based data
    glm::vec3 ownerTranslation = glm::vec3(0.0f);
    glm::vec3 focusTranslation = glm::vec3(0.0f);
  } data;

  WCameraComponent(ABase* pActor);

  void setTranslationOffset(float x, float y, float z, bool isDelta = false);
  void setTranslationOffset(const glm::vec3& newTranslation, bool isDelta = false);
  void setRotation(const glm::vec3& newRotation, bool isInRadians = false, bool isDelta = false);

  const glm::vec3& getTranslation();
  const glm::vec3& getTranslationOffset();
  const glm::vec3& getRotation();

  void setProjectionMode(ECameraProjection newMode);
  void setFOV(float newFOV);
  void setOrthoFOV(float newFOV);
  void setViewDistance(float newViewDistance);

  // Target actor must have transform component. Set focus mode to 'None' to free the camera.
  void setFocus(ECameraFocusMode newFocusMode, ABase* pTargetActor = nullptr);

  const ECameraProjection getProjectionMode();
  const float getFOV();
  const float getOrthoFOV();
  const float getViewDistance();
  const glm::vec2 getClipPlanes();

  const glm::mat4& getView();
  const glm::mat4& getProjection();

  void setIgnorePitchLimit(const bool newValue);
  bool getIsIgnoringPitchLimit();

  void setAspectRatio(float newValue) noexcept;
  float getAspectRatio() noexcept;

  void setViewBufferIndex(const uint32_t newIndex);
  uint32_t getViewBufferIndex();

  void onOwnerPossessed() override;
  void update() override;
  void drawComponentUI() override;

  // Event delegates
  void handleTransformUpdateEvent(const ComponentEvent& newEvent);
  void handleControllerRotation(const ComponentEvent& newEvent);
};