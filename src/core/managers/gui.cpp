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

core::MGUI::MGUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  m_pIO = &ImGui::GetIO();
  m_pIO->IniFilename = NULL;
  m_pIO->LogFilename = NULL;
}

void core::MGUI::setupEditor() {
  // Configure editor style
  m_pStyle = &ImGui::GetStyle();

  ImVec4* pColors = m_pStyle->Colors;
  pColors[ImGuiCol_WindowBg] = m_style.panelBackground;
  pColors[ImGuiCol_ChildBg] = m_style.panelBackground;
  pColors[ImGuiCol_PopupBg] = m_style.panelBackground;
  pColors[ImGuiCol_MenuBarBg] = m_style.panelTitle;

  pColors[ImGuiCol_TitleBg] = m_style.panelTitle;
  pColors[ImGuiCol_TitleBgActive] = m_style.panelTitleActive;
  pColors[ImGuiCol_TitleBgCollapsed] = m_style.panelTitleCollapsed;
  pColors[ImGuiCol_Text] = m_style.text;

  pColors[ImGuiCol_Header] = m_style.panelBackground;
  pColors[ImGuiCol_HeaderHovered] = m_style.panelHovered;
  pColors[ImGuiCol_HeaderActive] = m_style.panelActive;

  pColors[ImGuiCol_Button] = m_style.button;
  pColors[ImGuiCol_ButtonHovered] = m_style.buttonHovered;
  pColors[ImGuiCol_ButtonActive] = m_style.buttonActive;

  pColors[ImGuiCol_FrameBg] = m_style.panelBackground;
  pColors[ImGuiCol_FrameBgHovered] = m_style.panelHovered;

  pColors[ImGuiCol_Tab] = m_style.tab;
  pColors[ImGuiCol_TabHovered] = m_style.panelHovered;
  pColors[ImGuiCol_TabActive] = m_style.tabActive;

  m_pStyle->DisabledAlpha = m_style.panelDisabledTextAlpha;

  // Configure borders
  pColors[ImGuiCol_Border] = m_style.border;
  m_pStyle->WindowBorderSize = m_style.panelBorderSize;
  m_pStyle->PopupBorderSize = m_style.panelBorderSize;

  m_pStyle->ScrollbarSize = 12.0f;
  m_pStyle->ScrollbarRounding = 0.0f;
  m_pStyle->TabRounding = 0.0f;
  m_pStyle->WindowPadding = ImVec2(5, 5);
  m_pStyle->IndentSpacing = 24.0f;

  // Configure right panel
  m_editorData.rightPanelSize =
  { config::renderWidth * m_editorData.rightPanelScale, float(config::renderHeight) };
  m_editorData.rightPanelPosition = { config::renderWidth - m_editorData.rightPanelSize.x, 18.0f };

  m_editorData.menuBarSize = { m_editorData.rightPanelPosition.x, 0.0f };
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

      // Show FPS and frame time counter at the right side of the main menu bar
      m_util.updateTime += core::renderer.getFrameTime();

      if (m_util.updateTime > m_util.updateInterval) {
        m_util.FPS = core::renderer.getFPS();
        m_util.frameTime = core::renderer.getFrameTime();
        m_util.updateTime = 0.0f;
      }

      ImGui::SetCursorPosX(m_editorData.rightPanelPosition.x + 6.0f);
      ImGui::Text("FPS: %.1f (%.3f ms)", m_util.FPS, m_util.frameTime);

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
  ImGui::BeginChild("##TreeNodeParent", ImVec2(0, config::renderHeight * 0.35f), true);

  // Begin tree node list
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
  ImGui::BeginChild("##TreeNodeList", ImVec2(0, 0), ImGuiChildFlags_Border);

  if (drawTreeNode(core::ref.getSceneName(), true)) {
    if (drawTreeNode("Instances", true)) {
      // Models
      for (const auto& model : sceneGraph.instances) {
        if (drawTreeNode(model.first, true)) {
          // Instances
          for (const auto& instance : model.second) {
            if (drawTreeNode(instance.first)) {
              if (ImGui::IsItemClicked()) {
                selectSceneGraphItem(instance.first, ESceneGraphItemType::Instance);
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
        if (drawTreeNode(camera.first)) {
          if (ImGui::IsItemClicked()) {
            selectSceneGraphItem(camera.first, ESceneGraphItemType::Camera);
          }

          ImGui::TreePop();
        }
      }

      ImGui::TreePop();
    }

    if (drawTreeNode("Lights", true)) {
      for (const auto& light : sceneGraph.lights) {
        if (drawTreeNode(light.first)) {
          if (ImGui::IsItemClicked()) {
            selectSceneGraphItem(light.first, ESceneGraphItemType::Light);
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

    // Add more tabs if needed...

    ImGui::EndTabBar();
  }

  ImGui::EndChild();
}

void core::MGUI::drawActorProperties() {
}

void core::MGUI::drawSceneProperties() {
  ImGui::Text("Name: ");
  ImGui::SameLine();

  copyToTextBuffer(core::ref.getSceneName());

  if (ImGui::InputText("##SceneNameInput", m_util.textBuffer, m_util.textBufferSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
    // Enter pressed, do something with the input text (stored in buffer)
    printf("Entered text: %s\n", m_util.textBuffer);
  }
  ImGui::SetItemDefaultFocus();
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

void core::MGUI::selectSceneGraphItem(const std::string& name, ESceneGraphItemType itemType) {
  m_editorData.pSelectedActor = core::ref.getActor(name);
  m_editorData.actorType = itemType;
}

void core::MGUI::copyToTextBuffer(const std::string& text) {
  m_util.textBufferSize = (text.size() > 1024) ? 1024 : text.size();
  memcpy(m_util.textBuffer, text.data(), m_util.textBufferSize);
}

void core::MGUI::initialize() {
  RE_LOG(Log, "Initializing GUI manager.");

  // Connect imGUI to GLFW
  ImGui_ImplGlfw_InitForVulkan(core::window.getWindow(), true);

  // Prepare pipeline info for imGUI to use in a dynamic rendering pass later
  RTexture* pTexture = core::resources.getTexture(RTGT_IMGUI);

  VkPipelineRenderingCreateInfoKHR pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  pipelineInfo.viewMask = NULL;
  pipelineInfo.colorAttachmentCount = 1;
  pipelineInfo.pColorAttachmentFormats = &core::renderer.swapchain.pImages[0]->texture.imageFormat;
  pipelineInfo.pNext = nullptr;
  
  ImGui_ImplVulkan_InitInfo vulkanInfo{};
  vulkanInfo.Instance = core::renderer.APIInstance;
  vulkanInfo.PhysicalDevice = core::renderer.physicalDevice.device;
  vulkanInfo.Device = core::renderer.logicalDevice.device;
  vulkanInfo.DescriptorPool = core::renderer.system.descriptorPool;
  vulkanInfo.Queue = core::renderer.logicalDevice.queues.graphics;
  vulkanInfo.QueueFamily =
    static_cast<uint32_t>(core::renderer.physicalDevice.queueFamilyIndices.graphics[0]);
  vulkanInfo.PipelineCache = VK_NULL_HANDLE;
  vulkanInfo.ImageCount = core::renderer.swapchain.imageCount;
  vulkanInfo.MinImageCount = vulkanInfo.ImageCount;
  vulkanInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  vulkanInfo.UseDynamicRendering = true;
  vulkanInfo.PipelineRenderingCreateInfo = pipelineInfo;
  vulkanInfo.RenderPass = VK_NULL_HANDLE;
  vulkanInfo.Subpass = 0u;
  //vulkanInfo.Allocator = &core::renderer.memAlloc->m_AllocationCallbacks;
  vulkanInfo.Allocator = VK_NULL_HANDLE;
  vulkanInfo.CheckVkResultFn = nullptr;

  ImGui_ImplVulkan_Init(&vulkanInfo);

  ImGui_ImplVulkan_CreateFontsTexture();

  // Prepare dynamic rendering data
  core::renderer.gui.pTexture = pTexture;

  VkRenderingAttachmentInfo& attachmentInfo = core::renderer.gui.colorAttachment;
  attachmentInfo = VkRenderingAttachmentInfo{};
  attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  attachmentInfo.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};
  attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentInfo.imageView = pTexture->texture.view;
  attachmentInfo.imageLayout = pTexture->texture.imageLayout;
  attachmentInfo.pNext = nullptr;

  VkRenderingInfo& renderingInfo = core::renderer.gui.renderingInfo;
  renderingInfo = VkRenderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.pColorAttachments = &attachmentInfo;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.layerCount = pTexture->texture.layerCount;
  renderingInfo.renderArea.offset = { 0, 0 };
  renderingInfo.renderArea.extent.width = pTexture->texture.width;
  renderingInfo.renderArea.extent.height = pTexture->texture.height;
  renderingInfo.pDepthAttachment = nullptr;
  renderingInfo.pStencilAttachment = nullptr;
  renderingInfo.viewMask = NULL;
  renderingInfo.flags = NULL;
  renderingInfo.pNext = nullptr;

  if (config::bDevMode) {
    setupEditor();
  }
}

void core::MGUI::deinitialize() {
  core::renderer.waitForSystemIdle();
  ImGui_ImplVulkan_DestroyFontsTexture();
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

ImGuiIO& core::MGUI::io() {
  return *m_pIO;
}

void core::MGUI::render() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  switch (config::bDevMode) {
    case true: {
      if (m_isEditorVisible) {
        showEditor();
      }

#ifndef NDEBUG
      if (m_isImGUIDemoVisible) {
        ImGui::ShowDemoWindow();
      }
#endif

      break;
    }
    default: {
      break;
    }
  }

  ImGui::Render();
}
