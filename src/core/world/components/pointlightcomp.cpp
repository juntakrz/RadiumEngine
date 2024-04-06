#include "pch.h"
#include "core/core.h"
#include "core/managers/gui.h"
#include "core/managers/scene.h"
#include "core/world/actor.h"
#include "core/world/components/pointlightcomp.h"

WPointLightComponent::WPointLightComponent(WActor* pActor) {
  typeId = EComponentType::Light;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();
}

WPointLightComponent::~WPointLightComponent() {
  removeLightFromBuffer();
  pEvents->removeDelegates<TransformUpdateComponentEvent>(this);
}

void WPointLightComponent::setColor(const glm::vec4& newColor) {
  data.color = newColor;
}

const glm::vec4& WPointLightComponent::getColor() {
  return data.color;
}

void WPointLightComponent::setLocalTranslation(float x, float y, float z, const bool isDelta) {
  setLocalTranslation(glm::vec3(x, y, z), isDelta);
}

void WPointLightComponent::setLocalTranslation(const glm::vec3& newTranslation, const bool isDelta) {
  data.localTranslation = (isDelta) ? data.localTranslation + newTranslation : newTranslation;
  data.relativeTranslation = (data.ownerOrientation * data.localTranslation) * data.ownerScale;
}

const glm::vec3& WPointLightComponent::getLocalTranslation() {
  return data.localTranslation;
}

const glm::vec3& WPointLightComponent::getRelativeTranslation() {
  return data.relativeTranslation;
}

const glm::vec3 WPointLightComponent::getWorldTranslation() {
  return data.ownerTranslation + data.relativeTranslation;
}

void WPointLightComponent::setIsEnabled(const bool newValue) {
  data.isEnabled = newValue;
}

bool WPointLightComponent::getIsEnabled() {
  return data.isEnabled;
}

void WPointLightComponent::removeLightFromBuffer() {
  core::scene.unregisterPointLight(this);
}

void WPointLightComponent::onCreated() {
  pEvents->addDelegate<TransformUpdateComponentEvent>(this, &WPointLightComponent::handleTransformUpdateEvent);
  pOwner->forceUpdateTransform();
  core::scene.registerPointLight(this);
}

void WPointLightComponent::drawComponentUI(const uint32_t index) {
  const float availableWidth = ImGui::GetContentRegionAvail().x;
  bool removeComponent = false;

  glm::vec3 translation = getLocalTranslation();

  const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
    | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

  std::string label(core::gui.m_util.pointLightName + std::to_string(index));
  bool open = ImGui::TreeNodeEx(label.c_str(), treeNodeFlags);

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
    ImGui::Checkbox("Enable", &data.isEnabled);

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

    if (core::gui.drawFloatControl("Light intensity", intensity, 0.0f, 0.01f, "%.2f")) {
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
    core::gui.skipFrame();
    pOwner->removeComponent(this);
  }
}

void WPointLightComponent::handleTransformUpdateEvent(const ComponentEvent& newEvent) {
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