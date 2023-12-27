#pragma once

#include "core/world/actors/base.h"

class ALight : public ABase {
 private:
  EActorType m_typeId = EActorType::Light;
  ELightType m_lightType = ELightType::Directional;

  // r, g, b = color, a = intensity
  glm::vec4 m_lightProperties;

 public:
  void setLightColor(const glm::vec3& newColor);
  void setLightColor(float r, float g, float b);
  void setLightIntensity(float newIntensity);
  void setLightProperties(const glm::vec4& newProperties);

  const glm::vec4& getLightProperties();
  const glm::vec3 getLightColor();
  const float getLightIntensity();

  void setLightType(ELightType newType);
  ELightType getLightType();
};