#pragma once

namespace core {
class MGUI {
private:
  enum ESceneGraphItemType {
    Null,
    Instance,
    Camera,
    Light
  };

  struct {
    const ImGuiWindowFlags staticPanelFlags =
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    ImVec2 menuBarSize;

    ImVec2 rightPanelPosition;
    ImVec2 rightPanelSize;
    float rightPanelScale = 0.2f;

    // Currently selected scene graph entity
    ABase* pSelectedActor = nullptr;
    ESceneGraphItemType actorType = ESceneGraphItemType::Null;
  } m_editorData;

  struct {
    ImVec4 border = { 0.015f, 0.015f, 0.015f, 1.0f };

    ImVec4 panelBackground = { 0.008f, 0.008f, 0.008f, 1.0f };
    ImVec4 panelHovered = { 0.5f, 0.0f, 0.0f, 1.0f };
    ImVec4 panelActive = { 0.8f, 0.0f, 0.0f, 1.0f };
    ImVec4 panelTitle = { 0.0f, 0.0f, 0.0f, 1.0f };
    ImVec4 panelTitleActive = { 0.0f, 0.0f, 0.0f, 1.0f };
    ImVec4 panelTitleCollapsed = { 0.0f, 0.0f, 0.0f, 1.0f };

    ImVec4 button = { 0.08f, 0.08f, 0.08f, 1.0 };
    ImVec4 buttonHovered = { 0.5f, 0.00f, 0.00f, 1.0 };
    ImVec4 buttonActive = { 0.8f, 0.00f, 0.00f, 1.0 };

    ImVec4 nodeFolder = { 1.0f, 0.647f, 0.0f, 1.0f };

    ImVec4 tab = { 0.02f, 0.02f, 0.02f, 1.0f };
    ImVec4 tabActive = { 0.05f, 0.05f, 0.05f, 1.0f };

    ImVec4 text = { 1.0f, 1.0f, 1.0f, 1.0f };

    float panelDisabledTextAlpha = 0.2f;
    float panelBorderSize = 1.0f;
  } m_style;

  struct {
    const float updateInterval = 1.0f;
    float updateTime = 0.0f;

    float FPS = 0.0f;
    float frameTime = 0.0f;

    int32_t sceneGraphNodeIndex = 0;

    char textBuffer[1024];
    size_t textBufferSize = 0u;
  } m_util;

  ImGuiIO* m_pIO = nullptr;
  ImGuiStyle* m_pStyle = nullptr;
  ImGui::FileBrowser m_fileDialog;
  bool m_isEditorVisible = true;
  bool m_isImGUIDemoVisible = false;

  MGUI();
  void checkVulkanResult(VkResult result) {};
  
  void setupEditor();
  void showEditor();
  void drawSceneGraph();
  void drawScenePanel();
  void drawActorProperties();
  void drawSceneProperties();

  bool drawTreeNode(const std::string& name, const bool isFolder = false);

  void selectSceneGraphItem(const std::string& name, ESceneGraphItemType itemType);

  void copyToTextBuffer(const std::string& text);

public:
  static MGUI& get() {
    static MGUI _sInstance;
    return _sInstance;
  }

  void initialize();
  void deinitialize();

  ImGuiIO& io();

  void render();

  bool isEditorVisible();
};
}