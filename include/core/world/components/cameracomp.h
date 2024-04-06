#pragma once

#include "component.h"

struct WCameraComponent : public WComponent {
  struct {
    ECameraProjection projectionMode = ECameraProjection::Perspective;

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    glm::vec3 localTranslation = glm::vec3(0.0f);
    glm::vec3 relativeTranslation = localTranslation;

    float FOV = 75.0f;
    float orthoFOV = 1.0f;
    float nearPlane = RE_NEARZ;
    float viewDistance = 1000.0f;
    float aspectRatio = 1.0f;

    bool projectionRequiresUpdate = true;
    bool viewRequiresUpdate = true;

    uint32_t bufferViewIndex = -1;

    // Event based data
    glm::vec3 ownerTranslation = glm::vec3(0.0f);
    glm::quat ownerOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 ownerScale = glm::vec3(1.0f);
    glm::vec3 focusVector = glm::vec3(0.0f);
  } data;

  WCameraComponent() = default;
  WCameraComponent(WActor* pActor);
  ~WCameraComponent() override;

  void setLocalTranslation(float x, float y, float z, bool isDelta);
  void setLocalTranslation(const glm::vec3& newTranslation, bool isDelta);

  // Current camera location in the world, a sum of transform position and local camera offset
  const glm::vec3 getWorldTranslation();
  const glm::vec3& getLocalTranslation();
  const glm::vec3& getRelativeTranslation();

  void setProjectionMode(ECameraProjection newMode);
  void setFOV(float newFOV);
  void setOrthoFOV(float newFOV);
  void setViewDistance(float newViewDistance);
  void setAspectRatio(float newValue) noexcept;

  // Aspect ratio will be ignored if orthographic mode is selected
  void setCameraParameters(ECameraProjection newMode, float newFOV, float aspectRatio, float newViewDistance);
  void setCameraParameters(const RCameraInfo& info);

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

  void onAttachmentModeChanged(WActor* pNewTarget, EAttachmentMode newMode) override;
  void onCreated() override;

  void update() override;
  void drawComponentUI() override;

  // Event delegates
  void handleTransformUpdateEvent(const ComponentEvent& newEvent);
};