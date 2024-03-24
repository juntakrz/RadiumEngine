#include "pch.h"
#include "util/util.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/world/actors/base.h"
#include "core/world/actors/light.h"
#include "core/world/actors/camera.h"
#include "core/managers/gui.h"
#include "core/world/components/transform.h"

void WTransformComponent::showUIElement(ABase* pActor) {
  if (!pActor) {
    RE_LOG(Error, "%s: nullptr error.", __FUNCTION__);
    return;
  }

  const EActorType actorType = pActor->getTypeId();

  switch (actorType) {
    case EActorType::Camera: {
      break;
    }
    case EActorType::Light: {
      break;
    }
    default: {
      glm::vec3 translation = pActor->getTranslation();
      glm::vec3 rotation = pActor->getRotation();
      glm::vec3 scale = pActor->getScale();
      glm::vec3 deltaRotation = glm::degrees(rotation);

      const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
        | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
      ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_Header, core::gui.m_style.greyLow);
      bool open = ImGui::TreeNodeEx("Transform", treeNodeFlags);

      if (open) {
        core::gui.drawVec3Control("Translation", translation, core::gui.m_util.dragSensitivity);
        core::gui.drawVec3Control("Rotation", deltaRotation, core::gui.m_util.dragSensitivity * 10.0f, false, "%.2f");
        core::gui.drawVec3Control("Scale", scale, core::gui.m_util.dragSensitivity, core::gui.m_editorData.isTransformScaleLocked);

        ImVec2 lockButtonSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize("unlock").y + 5);
        ImGui::PushStyleColor(ImGuiCol_Button, (core::gui.m_editorData.isTransformScaleLocked)
          ? core::gui.m_style.redMedium : core::gui.m_style.greyLow);
        if (ImGui::Button((core::gui.m_editorData.isTransformScaleLocked) ? "Unlock scale" : "Lock scale", lockButtonSize)) {
          core::gui.m_editorData.isTransformScaleLocked = !core::gui.m_editorData.isTransformScaleLocked;
        }
        ImGui::PopStyleColor();

        pActor->setTranslation(translation);
        pActor->setScale(scale);

        deltaRotation = glm::radians(deltaRotation);
        deltaRotation -= rotation;

        pActor->rotate(deltaRotation, true);

        ImGui::Separator();

        ImGui::TreePop();
      }

      ImGui::PopStyleVar(2);
      ImGui::PopStyleColor();
    }
  }
}