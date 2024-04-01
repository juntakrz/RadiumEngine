#include "pch.h"
#include "util/math.h"
#include "core/core.h"
#include "core/managers/gui.h"
#include "core/managers/renderer.h"
#include "core/world/actors/base.h"
#include "core/world/components/transformcomp.h"
#include "core/world/components/cameracomp.h"

WCameraComponent::WCameraComponent(ABase* pActor) {
  typeId = EComponentType::Camera;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();

  // Subscribe to appropriate events
  pEvents->addDelegate<TransformUpdateComponentEvent>(this, &WCameraComponent::handleTransformUpdateEvent);
}

void WCameraComponent::setLocalTranslation(float x, float y, float z, bool isDelta) {
  setLocalTranslation(glm::vec3(x, y, z), isDelta);
}

void WCameraComponent::setLocalTranslation(const glm::vec3& newTranslation, bool isDelta) {
  data.localTranslation = (isDelta) ? data.localTranslation + newTranslation : newTranslation;
  data.relativeTranslation = (data.ownerOrientation * data.localTranslation) * data.ownerScale;
  data.viewRequiresUpdate = true;
}

const glm::vec3 WCameraComponent::getWorldTranslation() {
  return data.ownerTranslation + data.relativeTranslation;
}

const glm::vec3& WCameraComponent::getLocalTranslation() {
  return data.localTranslation;
}

const glm::vec3& WCameraComponent::getRelativeTranslation() {
  return data.relativeTranslation;
}

void WCameraComponent::setProjectionMode(ECameraProjection newMode) {
  data.projectionMode = newMode;
  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setFOV(float newFOV) {
  data.FOV = newFOV;
  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setOrthoFOV(float newFOV) {
  data.orthoFOV = newFOV;
  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setViewDistance(float newVideDistance) {
  data.viewDistance = newVideDistance;
  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setAspectRatio(float newValue) noexcept {
  data.aspectRatio = newValue;

  data.projectionRequiresUpdate = true;
}

void WCameraComponent::setCameraParameters(ECameraProjection newMode, float newFOV, float aspectRatio, float newViewDistance) {
  (newMode == ECameraProjection::Perspective) ? data.FOV = newFOV : data.orthoFOV = newFOV;

  if (newMode ==ECameraProjection::Perspective) {
    data.aspectRatio = aspectRatio;
  }

  data.projectionMode = newMode;
  data.viewDistance = newViewDistance;

  data.projectionRequiresUpdate = true;
}

const ECameraProjection WCameraComponent::getProjectionMode() {
  return data.projectionMode;
}

const float WCameraComponent::getFOV() {
  return data.FOV;
}

const float WCameraComponent::getOrthoFOV() {
  return data.orthoFOV;
}

const float WCameraComponent::getViewDistance() {
  return data.viewDistance;
}

const glm::vec2 WCameraComponent::getClipPlanes() {
  return { data.nearPlane, data.viewDistance };
}

float WCameraComponent::getAspectRatio() noexcept {
  return data.aspectRatio;
}

const glm::mat4& WCameraComponent::getView() {
  if (data.viewRequiresUpdate) {
    update();
  }

  return data.view;
}

const glm::mat4& WCameraComponent::getProjection() {
  if (data.projectionRequiresUpdate) {
    update();
  }

  return data.projection;
}

void WCameraComponent::setViewBufferIndex(const uint32_t newIndex) {
  data.bufferViewIndex = newIndex;
}

uint32_t WCameraComponent::getViewBufferIndex() {
  return data.bufferViewIndex;
}

void WCameraComponent::update() {
  if (data.projectionRequiresUpdate) {
    switch (data.projectionMode) {
      case ECameraProjection::Perspective: {
        data.projection = glm::perspective(glm::radians(data.FOV), data.aspectRatio, data.nearPlane, data.viewDistance);
        break;
      }

      case ECameraProjection::Orthographic: {
        data.projection = glm::ortho(-data.orthoFOV, data.orthoFOV, -data.orthoFOV, data.orthoFOV);
        break;
      }
    }

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

void WCameraComponent::drawComponentUI() {
  const float availableWidth = ImGui::GetContentRegionAvail().x;
  bool removeComponent = false;

  glm::vec3 translation = getLocalTranslation();

  const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
    | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

  bool open = ImGui::TreeNodeEx("Camera", treeNodeFlags);

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
    const char* projectionModes[] = { "Perspective", "Orthographic" };
    const uint8_t currentItem = (uint8_t)data.projectionMode;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, core::gui.m_style.black);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, core::gui.m_style.black);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, core::gui.m_style.black);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::BeginCombo("##Projection", projectionModes[currentItem], ImGuiComboFlags_None)) {
      for (uint8_t mode = 0; mode < IM_ARRAYSIZE(projectionModes); ++mode) {
        if (ImGui::Selectable(projectionModes[mode])) {
          setProjectionMode((ECameraProjection)mode);
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

    const float controlWidth = ImGui::GetContentRegionAvail().x * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 4 });

    if (core::gui.drawFloatControl("FOV", (data.projectionMode == ECameraProjection::Perspective)
                                   ? data.FOV : data.orthoFOV, controlWidth, 1.0f, "%.0f")) {
      data.projectionRequiresUpdate = true;
    }

    ImGui::SameLine();

    if (core::gui.drawFloatControl("VD", data.viewDistance, controlWidth, 1.0f, "%.0f")) {
      data.projectionRequiresUpdate = true;
    }

    ImGui::PopStyleVar();

    if (ImGui::Button("View this camera", {ImGui::GetContentRegionAvail().x, 0})) {
      core::renderer.setCamera(this, true);
    }

    ImGui::Separator();
    ImGui::TreePop();
  }

  ImGui::PopStyleVar(2);

  if (removeComponent) {
    if (core::renderer.getCamera() == this) {
      core::renderer.setCamera(core::renderer.getMainCamera(), true);
    }

    pOwner->removeComponent(this);
  }
}

void WCameraComponent::handleTransformUpdateEvent(const ComponentEvent& newEvent) {
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