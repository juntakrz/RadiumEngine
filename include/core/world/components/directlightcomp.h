#pragma once

#include "cameracomp.h"

struct WDirectLightComponent : public WCameraComponent {
  struct LightData {
    glm::vec4 color = glm::vec4(1.0f);  // xyz - color, w - intensity
  } lightData;

  WDirectLightComponent(WActor* pActor);

  void setLocalTranslation(float x, float y, float z, bool isDelta);
  void setLocalTranslation(const glm::vec3& newTranslation, bool isDelta);

  // Current camera location in the world, a sum of transform position and local camera offset
  const glm::vec3 getWorldTranslation();

  void setOrthoFOV(float newFOV);
  void setViewDistance(float newViewDistance);

  // Aspect ratio will be ignored if orthographic mode is selected
  void setCameraParameters(float newFOV, float newViewDistance);

  const float getOrthoFOV();
  const float getViewDistance();
  const glm::vec2 getClipPlanes();

  const glm::mat4& getView();
  const glm::mat4& getProjection();

  void setViewBufferIndex(const uint32_t newIndex);
  uint32_t getViewBufferIndex();

  void setColor(const glm::vec3& newColor);
  void setColor(const glm::vec4& newValue);
  void setIntensity(const float newIntensity);

  const glm::vec4& getColor();

  bool getIsEnabled();

  void onAttachmentModeChanged(WActor* pNewTarget, EAttachmentMode newMode) override;
  void onCreated() override;

  void update() override;
  void drawComponentUI(const uint32_t index) override;

  // Event delegates
  void handleTransformUpdateEvent(const ComponentEvent& newEvent);
};