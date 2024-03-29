#include "pch.h"
#include "core/core.h"
#include "core/managers/actors.h"
#include "core/world/actors/base.h"
#include "core/world/components/lightcomp.h"

WLightComponent::WLightComponent(ABase* pActor) {
  typeId = EComponentType::Light;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();
  pEvents->addDelegate<TransformUpdateComponentEvent>(this, &WLightComponent::handleTransformUpdateEvent);
  setLightType(ELightType::Point);
}

WLightComponent::~WLightComponent() {
  if (getLightType() == ELightType::Point) {
    core::actors.removePointLight(this);
  }

  pEvents->removeDelegate<TransformUpdateComponentEvent>(&WLightComponent::handleTransformUpdateEvent);
}

void WLightComponent::setLightType(ELightType newType) {
  if (data.lightType == newType) return;

  data.lightType = newType;

  switch (newType) {
    case ELightType::Directional: {
      //
      return;
    }
    case ELightType::Point: {
      core::actors.addPointLight(this);
      return;
    }
  }
}

ELightType WLightComponent::getLightType() {
  return data.lightType;
}

void WLightComponent::setColor(const glm::vec4& newColor) {
  data.color = newColor;
}

const glm::vec4& WLightComponent::getColor() {
  return data.color;
}

void WLightComponent::setTranslationOffset(const glm::vec3& newTranslation, const bool isDelta) {
  (isDelta) ? data.translationOffset += newTranslation : data.translationOffset = newTranslation;
}

const glm::vec3& WLightComponent::getTranslationOffset() {
  return data.translationOffset;
}

const glm::vec3 WLightComponent::getWorldTranslation() {
  return data.ownerTranslation + data.translationOffset;
}

void WLightComponent::setIsEnabled(const bool newValue) {
  data.isEnabled = newValue;
}

bool WLightComponent::getIsEnabled() {
  return data.isEnabled;
}

void WLightComponent::update() {
}

void WLightComponent::drawComponentUI() {
}

void WLightComponent::handleTransformUpdateEvent(const ComponentEvent& newEvent) {
}