#pragma once

#include "component.h"

struct WPointLightComponent : public WComponent {
  struct {
    glm::vec4 color = glm::vec4(1.0f);
    glm::vec3 localTranslation = glm::vec3(0.0f);
    glm::vec3 relativeTranslation = localTranslation;
    bool isEnabled = true;

    // Event based data
    glm::vec3 ownerTranslation = glm::vec3(0.0f);
    glm::quat ownerOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 ownerScale = glm::vec3(1.0f);
  } data;

  WPointLightComponent(WActor* pActor);
  ~WPointLightComponent() override;

  void setColor(const glm::vec4& newColor);
  const glm::vec4& getColor();

  void setLocalTranslation(float x, float y, float z, const bool isDelta);
  void setLocalTranslation(const glm::vec3& newTranslation, const bool isDelta);
  const glm::vec3& getLocalTranslation();
  const glm::vec3& getRelativeTranslation();
  const glm::vec3 getWorldTranslation();

  void setIsEnabled(const bool newValue);
  bool getIsEnabled();

  void removeLightFromBuffer();

  void onCreated() override;

  void drawComponentUI(const uint32_t index) override;

  // Event delegates
  void handleTransformUpdateEvent(const ComponentEvent& newEvent);
};