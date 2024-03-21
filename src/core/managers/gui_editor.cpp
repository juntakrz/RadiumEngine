#include "pch.h"
#include "util/util.h"
#include "core/core.h"
#include "core/managers/window.h"
#include "core/managers/actors.h"
#include "core/managers/debug.h"
#include "core/managers/ref.h"
#include "core/managers/renderer.h"
#include "core/managers/resources.h"
#include "core/model/model.h"
#include "core/managers/gui.h"

void core::MGUI::preprocessEditorData() {
  m_util.updateTime += core::renderer.getFrameTime();

  if (m_util.updateTime > m_util.updateInterval) {
    m_util.FPS = core::renderer.getFPS();
    m_util.frameTime = core::renderer.getFrameTime();
    m_util.updateTime = 0.0f;
  }

  const glm::ivec2& referenceRaycast = core::renderer.getRaycastPosition();
  if (referenceRaycast.x > -1 && referenceRaycast.y > -1) {
    m_util.referenceRaycast = referenceRaycast;
  }

  if (core::renderer.getSelectedActorUID() > -1) {
    selectSceneGraphItem(core::renderer.getSelectedActorUID());
  }
}

void core::MGUI::showEditor() {

  /* Main Menu */
  {
    if (ImGui::BeginMainMenuBar()) {
      // File menu
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New")) {
          // Handle new
        }

        if (ImGui::MenuItem("Open")) {
          m_fileDialog.SetTitle("Open scene");
          m_fileDialog.SetTypeFilters({ ".rscene" });
          m_fileDialog.Open();

          RE_LOG(Log, "Opening file dialog.");
        }

        if (ImGui::MenuItem("Save")) {
          // Handle map save
        }

        if (ImGui::MenuItem("Save as ...")) {
          m_fileDialog.SetTitle("Save scene");
          m_fileDialog.SetTypeFilters({ ".rscene" });
          m_fileDialog.Open();

          RE_LOG(Log, "Opening save file dialog.");
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit")) {
          // Handle exit
        }

        ImGui::EndMenu();
      }

#ifndef NDEBUG
      // Debug menu
      if (ImGui::BeginMenu("Debug")) {
        bool isRenderDocEnabled = core::debug.isRenderDocEnabled();
        bool isRenderDocOverlayVisible = core::debug.isRenderDocOverlayVisible();

        ImGui::Text(isRenderDocOverlayVisible ? "*" : "");
        ImGui::SameLine();
        if (ImGui::MenuItem("Show RenderDoc overlay", "", nullptr, isRenderDocEnabled)) {
          core::debug.enableRenderDocOverlay(!isRenderDocOverlayVisible);
        }

        ImGui::Text(m_isImGUIDemoVisible ? "*" : "");
        ImGui::SameLine();
        if (ImGui::MenuItem("Show ImGui demo window")) {
          m_isImGUIDemoVisible = !m_isImGUIDemoVisible;
        }

        ImGui::EndMenu();
      }
#endif

      drawFrameInfo();

      ImGui::EndMainMenuBar();
    }
  }

  /* Right Panel */
  {
    ImGui::SetNextWindowPos(m_editorData.rightPanelPosition);
    ImGui::SetNextWindowSize(m_editorData.rightPanelSize);

    ImGui::Begin("Scene Graph", nullptr, m_editorData.staticPanelFlags);

    drawSceneGraph();
    drawScenePanel();

    ImGui::End();
  }
  /* File Dialog */

  m_fileDialog.Display();

  // Handle file dialog actions
  if (m_fileDialog.HasSelected())
  {
    RE_LOG(Log, "Selected file: %s", m_fileDialog.GetSelected().string().c_str());
    m_fileDialog.ClearSelected();
  }
}

void core::MGUI::drawSceneGraph() {
  const auto& sceneGraph = core::ref.getSceneGraph();
  m_util.sceneGraphNodeIndex = 0;

  // Begin border area
  ImGui::BeginChild("##TreeNodeParent", ImVec2(0, config::renderHeight * 0.35f), ImGuiChildFlags_Border);

  // Begin tree node list
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
  ImGui::BeginChild("##TreeNodeList", ImVec2(0, 0), ImGuiChildFlags_Border);

  if (drawTreeNode(core::ref.getSceneName(), true)) {
    if (drawTreeNode("Instances", true)) {
      // Models
      for (const auto& model : sceneGraph.instances) {
        if (drawTreeNode(model.first->getName(), true)) {
          // Instances
          for (const auto& instance : model.second) {
            if (drawTreeNode(instance->getName())) {
              if (ImGui::IsItemClicked()) {
                selectSceneGraphItem(instance->getName(), ESceneGraphItemType::Instance);
              }

              ImGui::TreePop();
            }
          }

          ImGui::TreePop();
        }
      }

      ImGui::TreePop();
    }

    if (drawTreeNode("Cameras", true)) {
      for (const auto& camera : sceneGraph.cameras) {
        if (drawTreeNode(camera->getName())) {
          if (ImGui::IsItemClicked()) {
            selectSceneGraphItem(camera->getName(), ESceneGraphItemType::Camera);
          }

          ImGui::TreePop();
        }
      }

      ImGui::TreePop();
    }

    if (drawTreeNode("Lights", true)) {
      for (const auto& light : sceneGraph.lights) {
        if (drawTreeNode(light->getName())) {
          if (ImGui::IsItemClicked()) {
            selectSceneGraphItem(light->getName(), ESceneGraphItemType::Light);
          }

          ImGui::TreePop();
        }
      }

      ImGui::TreePop();
    }


    ImGui::TreePop();
  }

  // End tree node list
  ImGui::EndChild();
  ImGui::PopStyleVar();

  // End border area
  ImGui::EndChild();
}

void core::MGUI::drawScenePanel() {
  ImGui::BeginChild("##PropertiesParent", ImVec2(0, 0), ImGuiChildFlags_None);

  if (ImGui::BeginTabBar("##PropertiesTabBar")) {
    // Tab 1 - Actor properties
    if (ImGui::BeginTabItem("Actor")) {
      (m_editorData.pSelectedActor)
        ? drawActorProperties() : ImGui::Text("Nothing is currently selected.");

      ImGui::EndTabItem();
    }

    // Tab 2 - Scene properties
    if (ImGui::BeginTabItem("Scene")) {
      drawSceneProperties();

      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::EndChild();
}

void core::MGUI::drawActorProperties() {
  ImGui::BeginChild("##ActorPropertiesFrame", ImVec2(0, 0), ImGuiChildFlags_Border);

  ImGui::AlignTextToFramePadding();
  ImGui::Text("Name: ");
  ImGui::SameLine();

  copyToTextBuffer(m_editorData.pSelectedActor->getName());

  // Name input field
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, m_style.panelTitle); // Darker background color

  float inputTextWidth = ImGui::GetContentRegionAvail().x;
  ImGui::SetNextItemWidth(inputTextWidth);

  if (ImGui::InputText("##SceneNameInput", m_util.textBuffer, 64, ImGuiInputTextFlags_EnterReturnsTrue)) {
    m_editorData.pSelectedActor->setName(m_util.textBuffer);
  }

  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  // UID static field
  ImGui::Text("UID: %d", m_editorData.pSelectedActor->getUID());

  ImGui::EndChild();
}

void core::MGUI::drawSceneProperties() {
  ImGui::BeginChild("##ScenePropertiesFrame", ImVec2(0, 0), ImGuiChildFlags_Border);

  ImGui::AlignTextToFramePadding();
  ImGui::Text("Name: ");
  ImGui::SameLine();

  copyToTextBuffer(core::ref.getSceneName());

  // Push style for text input field
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, m_style.panelTitle); // Darker background color

  float inputTextWidth = ImGui::GetContentRegionAvail().x;
  ImGui::SetNextItemWidth(inputTextWidth);

  if (ImGui::InputText("##SceneNameInput", m_util.textBuffer, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
    core::ref.setSceneName(m_util.textBuffer);
  }

  // Pop style for text input field
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  ImGui::EndChild();
}

bool core::MGUI::drawTreeNode(const std::string& name, const bool isFolder) {
  ImGuiTreeNodeFlags flags = (isFolder)
    ? ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen
    : ImGuiTreeNodeFlags_Leaf;

  const std::string format = (isFolder) ? "[%s]" : "%s";

  if (isFolder) {
    ImGui::PushStyleColor(ImGuiCol_Text, m_style.nodeFolder);
  }

  bool result = ImGui::TreeNodeEx((void*)(intptr_t)m_util.sceneGraphNodeIndex, flags, format.c_str(), name.c_str());

  if (isFolder) {
    ImGui::PopStyleColor();
  }

  ++m_util.sceneGraphNodeIndex;
  return result;
}

void core::MGUI::drawFrameInfo() {
  // Show FPS and frame time counter at the right side of the main menu bar
  ImGui::SetCursorPosX(m_editorData.rightPanelPosition.x + 6.0f);
  ImGui::Text("FPS: %.1f (%.3f ms) Ray: %d, %d",
    m_util.FPS, m_util.frameTime, m_util.referenceRaycast.x, m_util.referenceRaycast.y);
}

void core::MGUI::selectSceneGraphItem(const std::string& name, ESceneGraphItemType itemType) {
  m_editorData.pSelectedActor = core::ref.getActor(name);
  m_editorData.actorType = itemType;

  //core::renderer.setSelectedActorUID(m_editorData.pSelectedActor->getUID());
}

void core::MGUI::selectSceneGraphItem(const int32_t UID) {
  m_editorData.pSelectedActor = core::ref.getActor(UID);

  switch (m_editorData.pSelectedActor->getTypeId()) {
    case EActorType::Entity:
    case EActorType::Pawn:
    case EActorType::Static:
      m_editorData.actorType = ESceneGraphItemType::Instance;
      return;

    case EActorType::Camera:
      m_editorData.actorType = ESceneGraphItemType::Camera;
      return;

    case EActorType::Light:
      m_editorData.actorType = ESceneGraphItemType::Light;
      return;
  }
}
