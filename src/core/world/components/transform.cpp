#include "pch.h"
#include "util/util.h"
#include "util/math.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/world/actors/base.h"
#include "core/world/actors/light.h"
#include "core/world/actors/camera.h"
#include "core/managers/gui.h"
#include "core/world/components/transform.h"

void WTransformComponent::showUIElement() {
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
          data.wasUpdated = true;
        }

        if (core::gui.drawVec3Control("Rotation", deltaRotation, core::gui.m_util.dragSensitivity * 10.0f, false, "%.2f")) {
          deltaRotation = glm::radians(deltaRotation);
          deltaRotation -= rotation;
          data.rotation += deltaRotation;
          data.orientation *= glm::quat(deltaRotation);
          math::wrapAnglesGLM(data.rotation);
          data.wasUpdated = true;
        }

        if (core::gui.drawVec3Control("Scale", scale, core::gui.m_util.dragSensitivity, core::gui.m_editorData.isTransformScaleLocked)) {
          data.scale = scale;
          data.wasUpdated = true;
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

glm::mat4& WTransformComponent::TransformComponentData::getModelTransformationMatrix(bool* updateResult) {
  if (wasUpdated) {
    // Translation * Rotation * Scaling
    transform = glm::mat4(1.0f);

    // Using SIMD copy instead of glm::translate
    util::copyVec3ToMatrix(&translation.x, transform, 3);

    // Using SIMD to multiply translated matrix by rotation and scaling matrices
    transform *= glm::mat4_cast(orientation) * glm::scale(scale);

    // Tell the caller that transformations were performed this call
    if (updateResult) *updateResult = true;

    wasUpdated = false;
  }

  return transform;
}