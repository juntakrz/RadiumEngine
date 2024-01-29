#pragma once

#include "core/world/actors/camera.h"

class ALight : public ACamera {
 protected:
  EActorType m_typeId = EActorType::Light;
  ELightType m_lightType = ELightType::Directional;

  // r, g, b = color, a = intensity
  glm::vec4 m_lightProperties;

  glm::mat4 m_projectionView = glm::mat4(1.0f);

  bool m_isShadowCaster = false;

 public:
  void setLightColor(const glm::vec3& newColor);
  void setLightColor(float r, float g, float b);
  void setLightIntensity(float newIntensity);
  void setLightProperties(const glm::vec4& newProperties);

  const glm::vec4& getLightProperties();
  const glm::vec3 getLightColor();
  const float getLightIntensity();

  glm::mat4 getLightProjectionView();

  void setAsShadowCaster(const bool isShadowCaster);
  bool isShadowCaster();

  void setLightType(ELightType newType);
  ELightType getLightType();
};