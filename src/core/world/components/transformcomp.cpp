#include "pch.h"
#include "util/util.h"
#include "util/math.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/world/actors/base.h"
#include "core/world/actors/light.h"
#include "core/world/actors/camera.h"
#include "core/managers/gui.h"
#include "core/world/components/componentevents.h"
#include "core/world/components/transformcomp.h"

WTransformComponent::WTransformComponent(ABase* pActor) {
  typeId = EComponentType::Transform;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();
}

const glm::mat4& WTransformComponent::getModelTransformationMatrix() {
  return data.transform;
}

void WTransformComponent::setTranslation(float x, float y, float z, bool isDelta) {
  data.translation.x = (isDelta) ? data.translation.x + x * data.deltaModifiers.x : x;
  data.translation.y = (isDelta) ? data.translation.y + y * data.deltaModifiers.y : y;
  data.translation.z = (isDelta) ? data.translation.z + z * data.deltaModifiers.z : z;

  data.requiresUpdate = true;
}

void WTransformComponent::setTranslation(const glm::vec3& newTranslation, bool isDelta) {
  data.translation = (isDelta) ? data.translation + newTranslation * data.deltaModifiers.x : newTranslation;

  data.requiresUpdate = true;
}

void WTransformComponent::setRotation(float x, float y, float z, bool isInRadians, bool isDelta) {
  setRotation(glm::vec3(x, y, z), isInRadians, isDelta);
}

void WTransformComponent::setRotation(const glm::vec3& newRotation, bool isInRadians, bool isDelta) {
  data.rotation = (isDelta)
    ? data.rotation + (((isInRadians) ? newRotation : glm::radians(newRotation)) * data.deltaModifiers.y)
    : (isInRadians) ? newRotation : glm::radians(newRotation);

  math::wrapAnglesGLM(data.rotation);

  data.orientation = (isDelta)
    ? data.orientation * glm::quat(((isInRadians) ? newRotation : glm::radians(newRotation)) * data.deltaModifiers.y)
    : glm::quat(data.rotation);

  data.requiresUpdate = true;
}

void WTransformComponent::setScale(float newScale, bool isDelta) {
  data.scale.x = (isDelta) ? data.scale.x + newScale * data.deltaModifiers.z : newScale;
  data.scale.y = (isDelta) ? data.scale.y + newScale * data.deltaModifiers.z : newScale;
  data.scale.z = (isDelta) ? data.scale.z + newScale * data.deltaModifiers.z : newScale;

  data.requiresUpdate = true;
}

void WTransformComponent::setScale(float x, float y, float z, bool isDelta) {
  data.scale.x = (isDelta) ? data.scale.x + x * data.deltaModifiers.z : x;
  data.scale.y = (isDelta) ? data.scale.y + y * data.deltaModifiers.z : y;
  data.scale.z = (isDelta) ? data.scale.z + z * data.deltaModifiers.z : z;

  data.requiresUpdate = true;
}

void WTransformComponent::setScale(const glm::vec3& newScale, bool isDelta) {
  data.scale = (isDelta) ? data.scale + newScale * data.deltaModifiers.z : newScale;

  data.requiresUpdate = true;
}

void WTransformComponent::setForwardVector(const glm::vec3& newForwardVector) {
  data.forwardVector = newForwardVector;

  data.requiresUpdate = true;
}

void WTransformComponent::setAbsoluteForwardVector(const glm::vec3& newForwardVector) {
  data.absoluteForwardVector = newForwardVector;

  data.requiresUpdate = true;
}

void WTransformComponent::setTranslationDeltaModifier(float newModifier) {
  data.deltaModifiers.x = newModifier;
}

void WTransformComponent::setRotationDeltaModifier(float newModifier) {
  data.deltaModifiers.y = newModifier;
}

void WTransformComponent::setScaleDeltaModifier(float newModifier) {
  data.deltaModifiers.z = newModifier;
}

const glm::vec3& WTransformComponent::getTranslation() {
  return data.translation;
}

const glm::vec3& WTransformComponent::getRotation() {
  return data.rotation;
}

const glm::quat& WTransformComponent::getOrientation() {
  return data.orientation;
}

const glm::vec3& WTransformComponent::getScale() {
  return data.scale;
}

const glm::vec3& WTransformComponent::getForwardVector() {
  return data.forwardVector;
}

const glm::vec3& WTransformComponent::getAbsoluteForwardVector() {
  return data.absoluteForwardVector;
}

const glm::vec3& WTransformComponent::getDeltaModifiers() {
  return data.deltaModifiers;
}

void WTransformComponent::onOwnerPossessed() {
  pEvents->addDelegate<ControllerTranslationComponentEvent>(this, &handleControllerTranslation);
  pEvents->addDelegate<ControllerRotationComponentEvent>(this, &handleControllerRotation);
}

void WTransformComponent::update() {
  if (data.requiresUpdate) {
    // Translation * Rotation * Scaling
    data.transform = glm::mat4(1.0f);

    // Using SIMD copy instead of glm::translate
    util::copyVec3ToMatrix(&data.translation.x, data.transform, 3);

    // Using SIMD to multiply translated matrix by rotation and scaling matrices
    data.transform *= glm::mat4_cast(data.orientation) * glm::scale(data.scale);

    data.requiresUpdate = false;

    // Generate a new event
    TransformUpdateComponentEvent newEvent;
    newEvent.pEventOwner = pOwner;
    newEvent.translation = data.translation;

    pEvents->addEvent<TransformUpdateComponentEvent>(newEvent);
  }
}

void WTransformComponent::drawComponentUI() {
  if (!pOwner) {
    RE_LOG(Error, "%s: nullptr error.", __FUNCTION__);
    return;
  }

  const EActorType actorType = pOwner->getTypeId();

  switch (actorType) {
  case EActorType::Camera: {
    break;
  }
  case EActorType::Light: {
    break;
  }
  default: {
    glm::vec3 translation = data.translation;
    glm::vec3 rotation = data.rotation;
    glm::vec3 scale = data.scale;
    glm::vec3 deltaRotation = glm::degrees(rotation);

    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
      | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Header, core::gui.m_style.greyLow);
    bool open = ImGui::TreeNodeEx("Transform", treeNodeFlags);

    if (open) {
      if (core::gui.drawVec3Control("Translation", translation, core::gui.m_util.dragSensitivity)) {
        data.translation = translation;
        data.requiresUpdate = true;
      }

      if (core::gui.drawVec3Control("Rotation", deltaRotation, core::gui.m_util.dragSensitivity * 10.0f, false, "%.2f")) {
        deltaRotation = glm::radians(deltaRotation);
        deltaRotation -= rotation;
        data.rotation += deltaRotation;
        data.orientation *= glm::quat(deltaRotation);
        math::wrapAnglesGLM(data.rotation);
        data.requiresUpdate = true;
      }

      if (core::gui.drawVec3Control("Scale", scale, core::gui.m_util.dragSensitivity, core::gui.m_editorData.isTransformScaleLocked)) {
        data.scale = scale;
        data.requiresUpdate = true;
      }

      ImVec2 lockButtonSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize("unlock").y + 5);
      ImGui::PushStyleColor(ImGuiCol_Button, (core::gui.m_editorData.isTransformScaleLocked)
        ? core::gui.m_style.redMedium : core::gui.m_style.greyLow);
      if (ImGui::Button((core::gui.m_editorData.isTransformScaleLocked) ? "Unlock scale" : "Lock scale", lockButtonSize)) {
        core::gui.m_editorData.isTransformScaleLocked = !core::gui.m_editorData.isTransformScaleLocked;
      }
      ImGui::PopStyleColor();

      ImGui::Separator();

      ImGui::TreePop();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
  }
  }
}

void WTransformComponent::handleControllerTranslation(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(ControllerTranslationComponentEvent)
    || newEvent.pEventOwner != pOwner) return;

  const ControllerTranslationComponentEvent componentEvent =
    static_cast<const ControllerTranslationComponentEvent&>(newEvent);

  setTranslation(componentEvent.controllerTranslationDelta, true);
}

void WTransformComponent::handleControllerRotation(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(ControllerRotationComponentEvent)
    || newEvent.pEventOwner != pOwner) return;

  const ControllerRotationComponentEvent componentEvent =
    static_cast<const ControllerRotationComponentEvent&>(newEvent);

  setRotation(componentEvent.controllerRotationDelta, true, true);
}
