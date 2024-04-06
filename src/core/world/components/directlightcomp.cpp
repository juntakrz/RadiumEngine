#include "pch.h"
#include "util/math.h"
#include "core/core.h"
#include "core/managers/gui.h"
#include "core/managers/renderer.h"
#include "core/managers/scene.h"
#include "core/world/actors/base.h"
#include "core/world/components/transformcomp.h"
#include "core/world/components/directlightcomp.h"

WDirectLightComponent::WDirectLightComponent(WActor* pActor) {
  data.projectionMode = ECameraProjection::Orthographic;
  typeId = EComponentType::DirectLight;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();
}

void WDirectLightComponent::setLocalTranslation(float x, float y, float z, bool isDelta) {
  setLocalTranslation(glm::vec3(x, y, z), isDelta);
}

void WDirectLightComponent::setLocalTranslation(const glm::vec3& newTranslation, bool isDelta) {
  data.localTranslation = (isDelta) ? data.localTranslation + newTranslation : newTranslation;
  data.relativeTranslation = (data.ownerOrientation * data.localTranslation) * data.ownerScale;
  data.viewRequiresUpdate = true;
}

const glm::vec3 WDirectLightComponent::getWorldTranslation() {
  return data.ownerTranslation + data.relativeTranslation;
}

void WDirectLightComponent::setOrthoFOV(float newFOV) {
  data.orthoFOV = newFOV;
  data.projectionRequiresUpdate = true;
}

void WDirectLightComponent::setViewDistance(float newVideDistance) {
  data.viewDistance = newVideDistance;
  data.projectionRequiresUpdate = true;
}

void WDirectLightComponent::setCameraParameters(float newFOV, float newViewDistance) {
  data.orthoFOV = newFOV;
  data.viewDistance = newViewDistance;

  data.projectionRequiresUpdate = true;
}

const float WDirectLightComponent::getOrthoFOV() {
  return data.orthoFOV;
}

const float WDirectLightComponent::getViewDistance() {
  return data.viewDistance;
}

const glm::vec2 WDirectLightComponent::getClipPlanes() {
  return { data.nearPlane, data.viewDistance };
}

const glm::mat4& WDirectLightComponent::getView() {
  if (data.viewRequiresUpdate) {
    update();
  }

  return data.view;
}

const glm::mat4& WDirectLightComponent::getProjection() {
  if (data.projectionRequiresUpdate) {
    update();
  }

  return data.projection;
}

void WDirectLightComponent::setViewBufferIndex(const uint32_t newIndex) {
  data.bufferViewIndex = newIndex;
}

uint32_t WDirectLightComponent::getViewBufferIndex() {
  return data.bufferViewIndex;
}

void WDirectLightComponent::setColor(const glm::vec3& newColor) {
  lightData.color.x = newColor.x;
  lightData.color.y = newColor.y;
  lightData.color.z = newColor.z;
}

void WDirectLightComponent::setColor(const glm::vec4& newValue) {
  lightData.color = newValue;
}

void WDirectLightComponent::setIntensity(const float newIntensity) {
  lightData.color.w = newIntensity;
}

const glm::vec4& WDirectLightComponent::getColor() {
  return lightData.color;
}

void WDirectLightComponent::onAttachmentModeChanged(WActor* pNewTarget, EAttachmentMode newMode) {
  pTarget = pNewTarget;
  attachmentMode = newMode;

  data.viewRequiresUpdate = true;
}

void WDirectLightComponent::onCreated() {
  pEvents->addDelegate<TransformUpdateComponentEvent>(this, &WDirectLightComponent::handleTransformUpdateEvent);
  core::scene.registerCamera(this);
}

void WDirectLightComponent::update() {
  if (data.projectionRequiresUpdate) {
    data.projection = glm::ortho(-data.orthoFOV, data.orthoFOV, -data.orthoFOV, data.orthoFOV, data.nearPlane, data.viewDistance);
    data.projectionRequiresUpdate = false;
  }

  if (data.viewRequiresUpdate) {
    switch (attachmentMode) {
    case EAttachmentMode::Translation: {
      data.view = glm::lookAt(
        getWorldTranslation(), getWorldTranslation() - data.focusVector,
        pOwner->getUpVector());
      break;
    }

    default: {
      data.view = glm::lookAt(
        getWorldTranslation(), getWorldTranslation() + pOwner->getForwardVector(),
        pOwner->getUpVector());
      break;
    }
    }

    data.viewRequiresUpdate = false;
  }
}

void WDirectLightComponent::drawComponentUI() {
  const float availableWidth = ImGui::GetContentRegionAvail().x;
  bool removeComponent = false;

  glm::vec3 translation = getLocalTranslation();

  const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
    | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

  bool open = ImGui::TreeNodeEx("Direct Light", treeNodeFlags);

  if (this != core::renderer.getMainCamera() && this != core::renderer.getEnvironmentCamera()) {
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
    const float controlWidth = ImGui::GetContentRegionAvail().x * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 4 });

    if (core::gui.drawFloatControl("FOV", data.orthoFOV, controlWidth, 1.0f, "%.0f")) {
      data.projectionRequiresUpdate = true;
    }

    ImGui::SameLine();

    if (core::gui.drawFloatControl("VD", data.viewDistance, controlWidth, 1.0f, "%.0f")) {
      data.projectionRequiresUpdate = true;
    }

    ImGui::PopStyleVar();

    float lightColor[4];
    memcpy(lightColor, lightData.color.data.m128_f32, sizeof(float) * 4);
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

    if (ImGui::Button("View this light", { ImGui::GetContentRegionAvail().x, 0 })) {
      core::renderer.setCamera(this, true);
    }

    if (ImGui::Button("Set this light", { ImGui::GetContentRegionAvail().x, 0 })) {
      core::renderer.setDirectionalLightCamera(this);
    }

    ImGui::Separator();
    ImGui::TreePop();
  }

  ImGui::PopStyleVar(2);

  if (removeComponent) {
    if (core::renderer.getDirectionalLightCamera() == this) {
      core::renderer.setDirectionalLightCamera((WDirectLightComponent*)nullptr);
    }

    pOwner->removeComponent(this);
  }
}

void WDirectLightComponent::handleTransformUpdateEvent(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(TransformUpdateComponentEvent)) {
    invalidComponentErrorMessage();
    return;
  }

  const TransformUpdateComponentEvent& componentEvent =
    static_cast<const TransformUpdateComponentEvent&>(newEvent);

  data.ownerTranslation = componentEvent.translation;
  data.ownerOrientation = componentEvent.orientation;
  data.ownerScale = componentEvent.scale;
  data.focusVector = componentEvent.attachmentVector;
  data.relativeTranslation = (data.ownerOrientation * data.localTranslation) * data.ownerScale;

  data.viewRequiresUpdate = true;
}