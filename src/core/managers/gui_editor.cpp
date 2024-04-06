#include "pch.h"
#include "util/util.h"
#include "util/math.h"
#include "core/core.h"
#include "core/managers/window.h"
#include "core/managers/debug.h"
#include "core/managers/scene.h"
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

  m_util.skipFrame = false;

  const glm::ivec2& referenceRaycast = core::renderer.getRaycastPosition();
  if (referenceRaycast.x > -1 && referenceRaycast.y > -1) {
    m_util.referenceRaycast = referenceRaycast;
  }

  switch (core::renderer.getSelectedActorUID()) {
    case -1: {
      deselectSceneGraphItem();
      break;
    }
    default: {
      selectSceneGraphItem(core::renderer.getSelectedActorUID());
      break;
    }
  }
}

void core::MGUI::showEditor() {
  drawMainMenu();

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

void core::MGUI::drawMainMenu() {
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

    if (ImGui::BeginMenu("Create")) {
      if (ImGui::MenuItem("Primitive")) {
        // Handle new primitive
      }

      if (ImGui::MenuItem("Import GLTF file")) {
        // Handle new primitive
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

void core::MGUI::drawSceneGraph() {
  const auto& sceneGraph = core::scene.getSceneGraph();
  m_util.sceneGraphNodeIndex = 0;

  // Begin border area
  ImGui::BeginChild("##TreeNodeParent", ImVec2(0, config::renderHeight * 0.35f), ImGuiChildFlags_Border);

  // Begin tree node list
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
  ImGui::BeginChild("##TreeNodeList", ImVec2(0, 0), ImGuiChildFlags_Border);

  if (drawTreeNode(core::scene.getSceneName(), true)) {
    if (drawTreeNode("Actors", true)) {
      for (const auto& actor : sceneGraph.actors) {
        if (drawTreeNode(actor->getName())) {
          if (ImGui::IsItemClicked()) {
            selectSceneGraphItem(actor->getName());
          }

          ImGui::TreePop();
        }
      }

      ImGui::TreePop();
    }

    if (drawTreeNode("Instances", true)) {
      // Models
      for (const auto& model : sceneGraph.instances) {
        if (drawTreeNode(model.first->getName(), true)) {
          // Instances
          for (const auto& instance : model.second) {
            if (drawTreeNode(instance->getName())) {
              if (ImGui::IsItemClicked()) {
                selectSceneGraphItem(instance->getName());
              }

              ImGui::TreePop();
            }
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
  WActor* pActor = m_editorData.pSelectedActor;
  if (!pActor) return;

  ImGui::BeginChild("##ActorPropertiesFrame", ImVec2(0, ImGui::GetContentRegionAvail().y - 16),
    NULL, ImGuiWindowFlags_AlwaysVerticalScrollbar);

  ImGui::AlignTextToFramePadding();
  ImGui::Text("Name: ");
  ImGui::SameLine();

  copyToTextBuffer(pActor->getName());

  // Name input field
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, m_style.black); // Darker background color

  float inputTextWidth = ImGui::GetContentRegionAvail().x;
  ImGui::SetNextItemWidth(inputTextWidth);

  if (ImGui::InputText("##SceneNameInput", m_util.textBuffer, 64, ImGuiInputTextFlags_EnterReturnsTrue)) {
    pActor->setName(m_util.textBuffer);
  }

  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  // UID static field
  ImGui::Text("UID: %d\tType: %s", pActor->getUID(), helper::actorTypeIdToText(pActor->getTypeId()).c_str());
  ImGui::Separator();

  if (ImGui::Button("Edit actor", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {

  }

  if (ImGui::Button("Add component", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
    ImGui::OpenPopup("##ComponentPopup");
  }

  drawAddComponent();

  ImGui::Separator();

  pActor->drawComponentUIElements();

  ImGui::EndChild();
}

void core::MGUI::drawSceneProperties() {
  ImGui::BeginChild("##ScenePropertiesFrame", ImVec2(0, 0));

  ImGui::AlignTextToFramePadding();
  ImGui::Text("Name: ");
  ImGui::SameLine();

  copyToTextBuffer(core::scene.getSceneName());

  // Push style for text input field
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, m_style.black); // Darker background color

  float inputTextWidth = ImGui::GetContentRegionAvail().x;
  ImGui::SetNextItemWidth(inputTextWidth);

  if (ImGui::InputText("##SceneNameInput", m_util.textBuffer, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
    core::scene.setSceneName(m_util.textBuffer);
  }

  // Pop style for text input field
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  ImGui::Separator();
  
  // 3D model list
  {
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth
      | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Header, m_style.greyLow);

    if (ImGui::TreeNodeEx("Scene models", treeNodeFlags)) {
      /*for (const auto& model : core::renderer.getModelReferences()) {
        drawTreeNode(model->getName());
      }*/

      ImGui::TreePop();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
  }

  ImGui::EndChild();
}

void core::MGUI::drawAddComponent() {
  WActor* pActor = m_editorData.pSelectedActor;
  if (!pActor) return;

  if (ImGui::BeginPopup("##ComponentPopup")) {
    if (ImGui::Selectable("Add new camera")) {
      pActor->addComponent<WCameraComponent>();
    }
    if (ImGui::Selectable("Add new direct light")) {
      pActor->addComponent<WDirectLightComponent>();
    }
    if (ImGui::Selectable("Add new point light")) {
      pActor->addComponent<WPointLightComponent>();
    }
    if (ImGui::Selectable("Add new model")) {

    }

    ImGui::EndPopup();
  }
}

bool core::MGUI::drawTreeNode(const std::string& name, const bool isFolder) {
  ImGuiTreeNodeFlags flags = (isFolder)
    ? ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen
    : ImGuiTreeNodeFlags_Leaf;

  const std::string format = (isFolder) ? "[%s]" : "%s";

  if (isFolder) {
    ImGui::PushStyleColor(ImGuiCol_Text, m_style.orange);
  }

  bool result = ImGui::TreeNodeEx((void*)(intptr_t)m_util.sceneGraphNodeIndex, flags, format.c_str(), name.c_str());

  if (isFolder) {
    ImGui::PopStyleColor();
  }

  ++m_util.sceneGraphNodeIndex;
  return result;
}

bool core::MGUI::drawFloatControl(const char* label, float& inOutValue, float width, float speed, const char* format) {
  bool result = false;
  const float textPadding = 10.0f;

  ImGui::PushID(label);
  ImVec2 textSize = ImGui::CalcTextSize(label);

  ImGui::PushStyleColor(ImGuiCol_Button, core::gui.m_style.greyMedium);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, core::gui.m_style.greyMedium);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, core::gui.m_style.greyMedium);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 4 });

  ImGui::Button(label, { textSize.x + textPadding, 21 });
  ImGui::SameLine();

  ImGui::PushStyleColor(ImGuiCol_Text, m_style.black);
  ImGui::PushItemWidth((width > 0.0f)
                        ? width - textSize.x - textPadding
                        : ImGui::GetContentRegionAvail().x - textPadding);

  if (ImGui::DragFloat("##X", &inOutValue, speed, 0.0f, 0.0f, format)) {
    result = true;
  }

  ImGui::PopItemWidth();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(4);
  ImGui::PopID();

  return result;
}

bool core::MGUI::drawVec3Control(const char* label, glm::vec3& vector,
  float speed, const bool locked, const char* format) {
  //auto boldFont = io.Fonts->Fonts[0];

  bool result = false;
  glm::vec3 originalVector = vector;

  ImGui::PushID(label);
  ImGui::Text(label);

  const float controlWidth = m_editorData.rightPanelSize.x * 0.78f;

  ImGui::PushMultiItemsWidths(3, controlWidth);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
  ImVec2 buttonSize = { 15, 21 };

  ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.1f, 0.1f, 1.0f });
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.8f, 0.1f, 0.1f, 1.0f });
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.1f, 1.0f });
  //ImGui::PushFont(boldFont);
  ImGui::Button("X", buttonSize);
  //ImGui::PopFont();
  ImGui::PopStyleColor(3);

  ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.0f, 0.0f, 1.0f });
  ImGui::SameLine();

  if (ImGui::DragFloat("##X", &vector.x, speed, 0.0f, 0.0f, format)) {
    result = true;

    if (locked) {
      originalVector.x = vector.x - originalVector.x;
      vector.y += originalVector.x;
      vector.z += originalVector.x;
    }
  }

  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::PopStyleColor();

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.8f, 0.1f, 1.0f });
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.1f, 0.8f, 0.1f, 1.0f });
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.8f, 0.1f, 1.0f });
  ImGui::Button("Y", buttonSize);
  ImGui::PopStyleColor(3);

  ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.0f, 0.0f, 1.0f });
  ImGui::SameLine();

  if (ImGui::DragFloat("##Y", &vector.y, speed, 0.0f, 0.0f, format)) {
    result = true;

    if (locked) {
      originalVector.y = vector.y - originalVector.y;
      vector.x += originalVector.y;
      vector.z += originalVector.y;
    }
  }

  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::PopStyleColor();

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.1f, 0.8f, 1.0f });
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.1f, 0.1f, 0.8f, 1.0f });
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.1f, 0.8f, 1.0f });
  ImGui::Button("Z", buttonSize);
  ImGui::PopStyleColor(3);

  ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.0f, 0.0f, 1.0f });
  ImGui::SameLine();

  if (ImGui::DragFloat("##Z", &vector.z, speed, 0.0f, 0.0f, format)) {
    result = true;

    if (locked) {
      originalVector.z = vector.z - originalVector.z;
      vector.x += originalVector.z;
      vector.y += originalVector.z;
    }
  }

  ImGui::PopItemWidth();
  ImGui::PopStyleColor();

  ImGui::PopStyleVar();

  //ImGui::Columns(1);

  ImGui::PopID();

  return result;
}

void core::MGUI::drawFrameInfo() {
  // Show FPS and frame time counter at the right side of the main menu bar
  ImGui::SetCursorPosX(m_editorData.rightPanelPosition.x + 6.0f);
  ImGui::Text("FPS: %.1f (%.2f ms) Ray: %d, %d",
    m_util.FPS, m_util.frameTime, m_util.referenceRaycast.x, m_util.referenceRaycast.y);
}

void core::MGUI::drawImGuizmo() {
  WCameraComponent* pCamera = core::renderer.getPrimaryCamera();
  if (!pCamera || !m_editorData.pSelectedActor
    || m_editorData.pSelectedActor->getComponent<WCameraComponent>() == pCamera) return;

  ECameraProjection projectionMode = pCamera->getProjectionMode();
  WTransformComponent* pTransform = m_editorData.pSelectedActor->getComponent<WTransformComponent>();

  ImGuizmo::SetOrthographic(projectionMode == ECameraProjection::Orthographic);
  ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
  ImGuizmo::SetRect(0.0f, 0.0f, (float)config::renderWidth, (float)config::renderHeight);

  if (ImGuizmo::IsUsing()) {
    ImGui::SetNextFrameWantCaptureKeyboard(true);
  }

  switch (m_editorData.transformMode) {
    case ImGuizmo::OPERATION::TRANSLATE: {
      glm::vec3 translation;

      ImGuizmo::SetGizmoSizeClipSpace(0.1f * m_editorData.guizmoScale);
      ImGuizmo::Enable(true);

      if (ImGuizmo::Manipulate_RE(glm::value_ptr(pCamera->getView()), glm::value_ptr(pCamera->getProjection()), ImGuizmo::TRANSLATE,
        ImGuizmo::WORLD, glm::value_ptr(pTransform->getModelTransformationMatrix()), &translation)) {

        pTransform->setWorldTranslation(translation, true);
      }

      break;
    }
    case ImGuizmo::OPERATION::ROTATE: {
      glm::quat orientation;

      ImGuizmo::SetGizmoSizeClipSpace(0.076f * m_editorData.guizmoScale);
      ImGuizmo::Enable(true);

      if (ImGuizmo::Manipulate_RE(glm::value_ptr(pCamera->getView()), glm::value_ptr(pCamera->getProjection()), ImGuizmo::ROTATE,
        ImGuizmo::LOCAL, glm::value_ptr(pTransform->getModelTransformationMatrix()), nullptr, &orientation)) {

        pTransform->setOrientation(orientation, true);
      }

      break;
    }
    case ImGuizmo::OPERATION::SCALE: {
      glm::vec3 scale;

      ImGuizmo::SetGizmoSizeClipSpace(0.1f * m_editorData.guizmoScale);
      ImGuizmo::Enable(true);

      if (ImGuizmo::Manipulate_RE(glm::value_ptr(pCamera->getView()), glm::value_ptr(pCamera->getProjection()), ImGuizmo::SCALE,
        ImGuizmo::WORLD, glm::value_ptr(pTransform->getModelTransformationMatrix()), &scale)) {

        pTransform->setScale(scale, false);
      }

      break;
    }
    default: {
      ImGuizmo::Enable(false);
      break;
    }
  }
}

void core::MGUI::selectSceneGraphItem(const std::string& name) {
  m_editorData.pSelectedActor = core::scene.getActor(name);
  core::renderer.setSelectedActorUID(m_editorData.pSelectedActor->getUID());
}

void core::MGUI::selectSceneGraphItem(const int32_t UID) {
  m_editorData.pSelectedActor = core::scene.getActor(UID);
  core::renderer.setSelectedActorUID(m_editorData.pSelectedActor->getUID());
}

void core::MGUI::deselectSceneGraphItem() {
  m_editorData.pSelectedActor = nullptr;
}