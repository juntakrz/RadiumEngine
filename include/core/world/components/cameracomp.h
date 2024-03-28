#pragma once

#include "component.h"

struct WCameraComponent : public WComponent {
  struct {
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    glm::vec3 translation = glm::vec3(0.0f);      // Translation from origin

    float FOV = 75.0f;
    float orthoFOV = 1.0f;
    float nearPlane = RE_NEARZ;
    float viewDistance = 1000.0f;
    float aspectRatio = 1.0f;

    ECameraFocusMode focusMode = ECameraFocusMode::None;
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

  // Current camera location in the world, a sum of transform position and local camera offset
  const glm::vec3 getTranslation();
  const glm::vec3& getTranslationOffset();

  void setProjectionMode(ECameraProjection newMode);
  void setFOV(float newFOV);
  void setOrthoFOV(float newFOV);
  void setViewDistance(float newViewDistance);
  void setAspectRatio(float newValue) noexcept;

  // Aspect ratio will be ignored if orthographic mode is selected
  void setCameraParameters(ECameraProjection newMode, float newFOV, float aspectRatio, float newViewDistance);

  // Target actor must have transform component. Set focus mode to 'None' to free the camera.
  void setFocus(ECameraFocusMode newFocusMode, ABase* pTargetActor = nullptr);

  const ECameraProjection getProjectionMode();
  const float getFOV();
  const float getOrthoFOV();
  const float getViewDistance();
  const glm::vec2 getClipPlanes();
  float getAspectRatio() noexcept;

  const glm::mat4& getView();
  const glm::mat4& getProjection();

  void setViewBufferIndex(const uint32_t newIndex);
  uint32_t getViewBufferIndex();

  //void onOwnerControlled() override;
  void update() override;
  void drawComponentUI() override;

  // Event delegates
  void handleTransformUpdateEvent(const ComponentEvent& newEvent);
};