#include "pch.h"
#include "core/core.h"
#include "core/managers/gui.h"
#include "core/managers/scene.h"
#include "core/world/actors/base.h"
#include "core/world/components/lightcomp.h"

WLightComponent::WLightComponent(ABase* pActor) {
  typeId = EComponentType::Light;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();
  pEvents->addDelegate<TransformUpdateComponentEvent>(this, &WLightComponent::handleTransformUpdateEvent);
  setLightMode(ELightMode::Point);
  pOwner->forceUpdateTransform();
}

WLightComponent::~WLightComponent() {
  removeLightFromBuffer();
  pEvents->removeDelegate<TransformUpdateComponentEvent>(&WLightComponent::handleTransformUpdateEvent);
}

void WLightComponent::setLightMode(ELightMode newType) {
  if (data.lightMode == newType) return;

  removeLightFromBuffer();
  data.lightMode = newType;

  switch (newType) {
    case ELightMode::Directional: {
      break;
    }
    case ELightMode::Point: {
      core::scene.registerPointLight(this);
      break;
    }
  }
}

ELightMode WLightComponent::getLightMode() {
  return data.lightMode;
}

void WLightComponent::setColor(const glm::vec4& newColor) {
  data.color = newColor;
}

const glm::vec4& WLightComponent::getColor() {
  return data.color;
}

void WLightComponent::setLocalTranslation(float x, float y, float z, const bool isDelta) {
  setLocalTranslation(glm::vec3(x, y, z), isDelta);
}

void WLightComponent::setLocalTranslation(const glm::vec3& newTranslation, const bool isDelta) {
  data.localTranslation = (isDelta) ? data.localTranslation + newTranslation : newTranslation;
  data.relativeTranslation = (data.ownerOrientation * data.localTranslation) * data.ownerScale;
}

const glm::vec3& WLightComponent::getLocalTranslation() {
  return data.localTranslation;
}

const glm::vec3& WLightComponent::getRelativeTranslation() {
  return data.relativeTranslation;
}

const glm::vec3 WLightComponent::getWorldTranslation() {
  return data.ownerTranslation + data.relativeTranslation;
}

void WLightComponent::setIsEnabled(const bool newValue) {
  data.isEnabled = newValue;
}

bool WLightComponent::getIsEnabled() {
  return data.isEnabled;
}

void WLightComponent::removeLightFromBuffer() {
  switch (data.lightMode) {
    case ELightMode::Directional: {
      if (this == core::scene.getDirectLight()) {
        core::scene.setDirectLight(nullptr);
      }
      return;
    }
    case ELightMode::Point: {
      core::scene.unregisterPointLight(this);
      return;
    }
  }
}

void WLightComponent::drawComponentUI() {
  const float availableWidth = ImGui::GetContentRegionAvail().x;
  bool removeComponent = false;

  glm::vec3 translation = getLocalTranslation();

  const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
    | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

  bool open = ImGui::TreeNodeEx("Light", treeNodeFlags);

  {
    ImGui::SameLine(availableWidth - 15.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, core::gui.m_style.greyMedium);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, core::gui.m_style.greyLow);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, core::gui.m_style.greyLow);


    if (ImGui::Button("+")) {
      ImGui::OpenPopup("##ComponentOptions");
    }

    if (ImGui::BeginPopup("##ComponentOptions", ImGuiWindowFlags_NoMove)) {
      if (ImGui::Selectable("Remove component")) {
        removeComponent = true;
      }

      ImGui::EndPopup();
    }

    ImGui::PopStyleColor(3);
  }

  if (open) {
    const char* lightModes[] = { "Directional", "Point" };
    const uint8_t currentItem = (uint8_t)data.lightMode;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, core::gui.m_style.black);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, core::gui.m_style.black);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, core::gui.m_style.black);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::BeginCombo("##LightMode", lightModes[currentItem], ImGuiComboFlags_None)) {
      for (uint8_t mode = 0; mode < IM_ARRAYSIZE(lightModes); ++mode) {
        if (ImGui::Selectable(lightModes[mode])) {
          setLightMode((ELightMode)mode);
        }
      }

      ImGui::EndCombo();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    if (core::gui.drawVec3Control("Local translation", translation, core::gui.m_util.dragSensitivity)) {
      setLocalTranslation(translation, false);
    }

    ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 20);
    if (ImGui::Button("Reset translation", buttonSize)) {
      setLocalTranslation(glm::vec3(0.0f), false);
    }

    float lightColor[4];
    memcpy(lightColor, data.color.data.m128_f32, sizeof(float) * 4);
    float intensity = lightColor[3];
    bool changedColor = false;

    ImGui::PushStyleColor(ImGuiCol_Text, core::gui.m_style.black);

    if (ImGui::ColorPicker3("##Light color", lightColor, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoTooltip)) {
      changedColor = true;
    }

    ImGui::PopStyleColor();

    if (core::gui.drawFloatControl("Light intensity", intensity, 0.0f, 0.1f, "%.1f")) {
      changedColor = true;
    }

    if (changedColor) {
      setColor(glm::vec4(lightColor[0], lightColor[1], lightColor[2], intensity));
    }

    ImGui::Separator();
    ImGui::TreePop();
  }

  ImGui::PopStyleVar(2);

  if (removeComponent) {
    pOwner->removeComponent(this);
  }
}

void WLightComponent::handleTransformUpdateEvent(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(TransformUpdateComponentEvent)) {
    invalidComponentErrorMessage();
    return;
  }

  const TransformUpdateComponentEvent& componentEvent =
    static_cast<const TransformUpdateComponentEvent&>(newEvent);

  data.ownerTranslation = componentEvent.translation;
  data.ownerOrientation = componentEvent.orientation;
  data.ownerScale = componentEvent.scale;
  data.relativeTranslation = (data.ownerOrientation * data.localTranslation) * data.ownerScale;
}