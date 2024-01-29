#include "pch.h"
#include "core/world/actors/light.h"

void ALight::setLightColor(const glm::vec3& newColor) {
  m_lightProperties = glm::vec4(newColor, m_lightProperties.a);
}

void ALight::setLightColor(float r, float g, float b) {
  m_lightProperties.r = r;
  m_lightProperties.g = g;
  m_lightProperties.b = b;
}

void ALight::setLightIntensity(float newIntensity) {
  m_lightProperties.a = newIntensity;
}

void ALight::setLightProperties(const glm::vec4& newProperties) {
  m_lightProperties = newProperties;
}

const glm::vec4& ALight::getLightProperties() { return m_lightProperties; }

const glm::vec3 ALight::getLightColor() {
  return {m_lightProperties.r, m_lightProperties.g, m_lightProperties.b};
}

const float ALight::getLightIntensity() { return m_lightProperties.a; }

glm::mat4 ALight::getLightProjectionView() {
  if (m_transformationData.wasUpdated) {
    m_projectionView = getProjection() * getView();
    m_transformationData.wasUpdated = true;
  }

  return m_projectionView;
}

void ALight::setAsShadowCaster(const bool isShadowCaster) {
  m_isShadowCaster = isShadowCaster;
}

bool ALight::isShadowCaster() {
  return m_isShadowCaster;
}

void ALight::setLightType(ELightType newType) { m_lightType = newType; }

ELightType ALight::getLightType() { return m_lightType; }
