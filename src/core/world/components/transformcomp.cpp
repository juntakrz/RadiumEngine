#include "pch.h"
#include "util/util.h"
#include "util/math.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/gui.h"
#include "core/world/actor.h"
#include "core/world/components/componentevents.h"
#include "core/world/components/transformcomp.h"

WTransformComponent::WTransformComponent(WActor* pActor) {
  typeId = EComponentType::Transform;
  pOwner = pActor;
  pEvents = &pOwner->getEventSystem();
}

glm::mat4& WTransformComponent::getModelTransformationMatrix() {
  return data.transform;
}

void WTransformComponent::setTranslation(float x, float y, float z, bool isDelta) {
  setTranslation(glm::vec3(x, y, z), isDelta);
}

void WTransformComponent::setTranslation(const glm::vec3& newTranslation, bool isDelta) {
  // If delta - the actor is moving - so use orientation to translate
  switch (isDelta) {
    case true: {
      data.translation += data.orientation * newTranslation * data.deltaModifiers.x;
      break;
    }

    case false: {
      data.translation = newTranslation;
      break;
    }
  }

  data.transformRequiresUpdate = true;
}

void WTransformComponent::setWorldTranslation(const glm::vec3& newTranslation, bool isDelta) {
  data.translation = (isDelta) ? data.translation + newTranslation : newTranslation;

  data.transformRequiresUpdate = true;
}

void WTransformComponent::setRotation(float x, float y, float z, bool isInRadians, bool isDelta) {
  setRotation(glm::vec3(x, y, z), isInRadians, isDelta);
}

void WTransformComponent::setRotation(const glm::vec3& newRotation, bool isInRadians, bool isDelta) {
  switch (data.controlMode) {
    case EActorControlMode::Spacecraft: {
      /*data.rotation = (isDelta)
        ? data.rotation + (((isInRadians) ? newRotation : glm::radians(newRotation)) * data.deltaModifiers.y)
        : (isInRadians) ? newRotation : glm::radians(newRotation);

      math::wrapAnglesGLM(data.rotation);

      data.orientation = (isDelta)
        ? data.orientation * glm::normalize(glm::quat(((isInRadians) ? newRotation : glm::radians(newRotation) * data.deltaModifiers.y)))
        : glm::quat(data.rotation);*/

      //
      const glm::vec3& rotation = (isInRadians) ? newRotation : glm::radians(newRotation);

      data.orientation = (isDelta)
        ? glm::normalize(data.orientation * glm::quat(rotation * data.deltaModifiers.y))
        : glm::quat(rotation);

      data.rotation = glm::eulerAngles(data.orientation);
      //

      pOwner->setForwardVector(data.orientation * pOwner->getDefaultForwardVector());
      pOwner->setUpVector(data.orientation * pOwner->getDefaultUpVector());
      break;
    }

    case EActorControlMode::FirstPerson:
    case EActorControlMode::ThirdPerson: {
      switch (isDelta) {
        case true: {
          float newPitch = data.rotation.x;
          newPitch += (isInRadians) ? newRotation.x : glm::radians(newRotation.x);

          // Do not apply any changes if delta rotation reaches pitch limit to avoid abrupt camera jumps
          if (newPitch > config::pitchLimit || newPitch < -config::pitchLimit) return;

          int8_t direction = (newRotation.x != 0.0f) ? 0 : 1;

          data.rotation += (isInRadians) ? newRotation : glm::radians(newRotation);
          math::wrapAnglesGLM(data.rotation);
          data.orientation = (direction == 0)
            ? data.orientation * glm::quat((isInRadians) ? newRotation : glm::radians(newRotation)) * data.deltaModifiers.y
            : glm::quat((isInRadians) ? newRotation : glm::radians(newRotation))* data.deltaModifiers.y* data.orientation;

          pOwner->setForwardVector(data.orientation * pOwner->getDefaultForwardVector());
          break;
        }

        case false: {
          glm::vec3 rotation = (isInRadians) ? newRotation : glm::radians(newRotation);
          
          (rotation.x > config::pitchLimit)
            ? rotation.x = config::pitchLimit : (rotation.x < -config::pitchLimit)
              ? rotation.x = -config::pitchLimit : 0;

          data.rotation = rotation;
          math::wrapAnglesGLM(data.rotation);
          data.orientation = glm::quat(data.rotation);
          pOwner->setForwardVector(data.orientation * pOwner->getDefaultForwardVector());
          break;
        }
      }

      break;
    }
  }

  data.transformRequiresUpdate = true;
}

void WTransformComponent::setOrientation(const glm::quat& newQuat, const bool isDelta) {
  data.orientation = (isDelta) ? data.orientation * newQuat : newQuat;
  data.rotation = glm::eulerAngles(data.orientation);

  data.transformRequiresUpdate = true;
}

void WTransformComponent::setScale(float newScale, bool isDelta) {
  data.scale.x = (isDelta) ? data.scale.x + newScale * data.deltaModifiers.z : newScale;
  data.scale.y = (isDelta) ? data.scale.y + newScale * data.deltaModifiers.z : newScale;
  data.scale.z = (isDelta) ? data.scale.z + newScale * data.deltaModifiers.z : newScale;

  data.transformRequiresUpdate = true;
}

void WTransformComponent::setScale(float x, float y, float z, bool isDelta) {
  data.scale.x = (isDelta) ? data.scale.x + x * data.deltaModifiers.z : x;
  data.scale.y = (isDelta) ? data.scale.y + y * data.deltaModifiers.z : y;
  data.scale.z = (isDelta) ? data.scale.z + z * data.deltaModifiers.z : z;

  data.transformRequiresUpdate = true;
}

void WTransformComponent::setScale(const glm::vec3& newScale, bool isDelta) {
  data.scale = (isDelta) ? data.scale + newScale * data.deltaModifiers.z : newScale;

  data.transformRequiresUpdate = true;
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

const glm::vec3& WTransformComponent::getDeltaModifiers() {
  return data.deltaModifiers;
}

void WTransformComponent::setAttachmentVectorRotation(const glm::vec3& newRotation, const bool isInRadians, const bool isDelta) {
  data.attachmentOrientation = (isDelta)
    ? data.attachmentOrientation * glm::quat(((isInRadians) ? newRotation : glm::radians(newRotation)) * data.deltaModifiers.y)
    : glm::quat((isInRadians) ? newRotation : glm::radians(newRotation));
  data.attachmentOrientation = glm::normalize(data.attachmentOrientation);
  data.attachmentVector = data.attachmentOrientation * data.baseAttachmentVector;
  setTranslation(data.attachmentTranslation + data.attachmentVector, false);
}

void WTransformComponent::setAttachmentVectorLength(const float newLength) {
  data.baseAttachmentVector.z = newLength;
  setAttachmentVectorRotation(glm::vec3(0.0f), true, true);
}

const glm::vec3& WTransformComponent::getAttachmentVector() {
  return data.attachmentVector;
}

void WTransformComponent::forceUpdateTransform() {
  data.transformRequiresUpdate = true;
}

void WTransformComponent::onAttachmentModeChanged(WActor* pNewTarget, EAttachmentMode newMode) {
  if (attachmentMode == newMode || (newMode != EAttachmentMode::None && !pNewTarget)) return;

  switch (newMode) {
    case EAttachmentMode::None: {
      if (pTarget) {
          pTarget->getEventSystem().removeDelegates<TransformUpdateComponentEvent>(this);
          pTarget->getEventSystem().removeDelegates<ActorDestroyedComponentEvent>(this);
          pTarget = nullptr;
          data.attachmentVector = glm::vec3(0.0f);
      }

      break;
    }
    case EAttachmentMode::Translation:
    case EAttachmentMode::TranslationAndRotation: {
      pTarget = pNewTarget;
      pTarget->getEventSystem().addDelegate<TransformUpdateComponentEvent>(this, &WTransformComponent::handleAttachmentTargetTransformUpdated);
      pTarget->getEventSystem().addDelegate<ActorDestroyedComponentEvent>(this, &WTransformComponent::handleAttachmentTargetDestroyed);

      // Initialize attachment transform data
      data.baseAttachmentVector = glm::vec3(0.0f, 0.0f, -1.0f) * glm::compMax(pTarget->getScale());
      setAttachmentVectorRotation(glm::vec3(0.0f), true, false);
      data.translation = pTarget->getTranslation() + data.attachmentVector;
      break;
    }
  }

  attachmentMode = newMode;
}

void WTransformComponent::onOwnerControlled() {
  pEvents->addDelegate<ControllerTranslationComponentEvent>(this, &WTransformComponent::handleControllerTranslation);
  pEvents->addDelegate<ControllerRotationComponentEvent>(this, &WTransformComponent::handleControllerRotation);

  data.controlMode = pOwner->getControlMode();
}

void WTransformComponent::onOwnerFreed() {
  pEvents->removeDelegates<ControllerTranslationComponentEvent>(this);
  pEvents->removeDelegates<ControllerRotationComponentEvent>(this);
}

void WTransformComponent::onOwnerUpdated() {
  data.controlMode = pOwner->getControlMode();
}

void WTransformComponent::update() {
  if (data.transformRequiresUpdate) {
    // Translation * Rotation * Scaling
    data.transform = glm::mat4(1.0f);

    // Using SIMD copy instead of glm::translate
    util::copyVec3ToMatrix(&data.translation.x, data.transform, 3);

    data.scale = glm::max(data.scale, glm::vec3(0.001f));
    data.transform = data.transform * glm::mat4_cast(data.orientation) * glm::scale(data.scale);

    data.transformRequiresUpdate = false;

    // Generate a new event
    TransformUpdateComponentEvent newEvent;
    newEvent.pEventOwner = pOwner;
    newEvent.translation = data.translation;
    newEvent.rotation = data.rotation;
    newEvent.orientation = data.orientation;
    newEvent.scale = data.scale;
    newEvent.attachmentVector = data.attachmentVector;

    pEvents->sendEvent<TransformUpdateComponentEvent>(newEvent);
  }
}

void WTransformComponent::drawComponentUI(const uint32_t index) {
  glm::vec3 translation = (attachmentMode != EAttachmentMode::None)
    ? data.attachmentVector : data.translation;
  glm::vec3 rotation = (attachmentMode != EAttachmentMode::None)
    ? glm::eulerAngles(data.attachmentOrientation) : data.rotation;
  glm::vec3 scale = data.scale;
  glm::vec3 deltaRotation = glm::degrees(rotation);

  const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
    | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

  bool open = ImGui::TreeNodeEx("Transform", treeNodeFlags);

  if (open) {
    switch (attachmentMode) {
      case EAttachmentMode::Translation:
      case EAttachmentMode::TranslationAndRotation: {
        float vectorLength = data.baseAttachmentVector.z;

        ImGui::Text("Attached to: '%s'.", pTarget->getName().c_str());
        if (core::gui.drawFloatControl("Vector length", vectorLength, 0.0f, 0.01f, "%.2f")) {
          setAttachmentVectorLength(vectorLength);
        }

        core::gui.drawVec3Control("Translation", translation, core::gui.m_util.dragSensitivity);

        if (core::gui.drawVec3Control("Rotation", deltaRotation, core::gui.m_util.dragSensitivity * 10.0f, false, "%.2f")) {
          deltaRotation = glm::radians(deltaRotation);
          deltaRotation -= rotation;
          setAttachmentVectorRotation(deltaRotation, true, true);
        }

        break;
      }

      default: {
        if (core::gui.drawVec3Control("Translation", translation, core::gui.m_util.dragSensitivity)) {
          setTranslation(translation, false);
        }

        if (core::gui.drawVec3Control("Rotation", deltaRotation, core::gui.m_util.dragSensitivity * 10.0f, false, "%.2f")) {
          deltaRotation = glm::radians(deltaRotation);
          deltaRotation -= rotation;
          setRotation(deltaRotation, true, true);
        }

        break;
      }
    }

    if (core::gui.drawVec3Control("Scale", scale, core::gui.m_util.dragSensitivity, core::gui.m_editorData.isTransformScaleLocked)) {
      setScale(scale, false);
    }

    ImVec2 lockButtonSize = ImVec2(ImGui::GetContentRegionAvail().x, 20);
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
}

void WTransformComponent::handleControllerTranslation(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(ControllerTranslationComponentEvent)
    || newEvent.pEventOwner != pOwner) {
    invalidComponentErrorMessage();
    return;
  }

  const ControllerTranslationComponentEvent& componentEvent =
    static_cast<const ControllerTranslationComponentEvent&>(newEvent);

  setTranslation(componentEvent.controllerTranslationDelta, true);
}

void WTransformComponent::handleControllerRotation(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(ControllerRotationComponentEvent)
    || newEvent.pEventOwner != pOwner) {
    invalidComponentErrorMessage();
    return;
  }

  const ControllerRotationComponentEvent& componentEvent =
    static_cast<const ControllerRotationComponentEvent&>(newEvent);

  setRotation(componentEvent.controllerRotationDelta, true, true);
}

void WTransformComponent::handleAttachmentTargetTransformUpdated(const ComponentEvent& newEvent) {
  if (typeid(newEvent) != typeid(TransformUpdateComponentEvent)) {
    invalidComponentErrorMessage();
    return;
  }

  const TransformUpdateComponentEvent& componentEvent =
    static_cast<const TransformUpdateComponentEvent&>(newEvent);

  switch (attachmentMode) {
    case EAttachmentMode::Translation: {
      data.attachmentTranslation = componentEvent.translation;
      setTranslation(data.attachmentTranslation + data.attachmentVector, false);
      return;
    }
  }
}

void WTransformComponent::handleAttachmentTargetDestroyed(const ComponentEvent& newEvent) {
  if (pTarget) {
    attachmentMode = EAttachmentMode::None;
    pTarget = nullptr;
  }
}