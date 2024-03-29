#pragma once

#include "component.h"

struct WLightComponent : public WComponent {
  struct {
    ELightType lightType = ELightType::Point;
    glm::vec4 color = glm::vec4(1.0f);
    glm::vec3 translationOffset = glm::vec3(0.0f);
    bool isShadowCaster = false;
    bool isEnabled = true;

    // Event based data
    glm::vec3 ownerTranslation = glm::vec3(0.0f);
    glm::quat ownerOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  } data;

  WLightComponent(ABase* pActor);
  ~WLightComponent();

  void setLightType(ELightType newType);
  ELightType getLightType();

  void setColor(const glm::vec4& newColor);
  const glm::vec4& getColor();

  void setTranslationOffset(const glm::vec3& newTranslation, const bool isDelta);
  const glm::vec3& getTranslationOffset();
  const glm::vec3 getWorldTranslation();

  void setIsEnabled(const bool newValue);
  bool getIsEnabled();

  void update() override;
  void drawComponentUI() override;

  // Event delegates
  void handleTransformUpdateEvent(const ComponentEvent& newEvent);
};