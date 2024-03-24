#pragma once

namespace core {
class MGUI {
public:
  enum class ESceneGraphItemType : uint8_t {
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

    bool isTransformScaleLocked = false;
  } m_editorData;

  struct EditorStyle {
    ImVec4 black = { 0.0f, 0.0f, 0.0f, 1.0f };
    ImVec4 greyDark = { 0.008f, 0.008f, 0.008f, 1.0f };
    ImVec4 greyMedium = { 0.08f, 0.08f, 0.08f, 1.0f };
    ImVec4 greyLow = { 0.033f, 0.033f, 0.033f, 1.0f };
    ImVec4 orange = { 1.0f, 0.647f, 0.0f, 1.0f };
    ImVec4 redMedium = { 0.5f, 0.01f, 0.01f, 1.0f };
    ImVec4 redBright = { 0.8f, 0.01f, 0.01f, 1.0f };
    ImVec4 white = { 1.0f, 1.0f, 1.0f, 1.0f };

    float panelDisabledTextAlpha = 0.2f;
    float panelBorderSize = 1.0f;
  } m_style;

  struct {
    const float updateInterval = 1.0f;
    float updateTime = 0.0f;

    float FPS = 0.0f;
    float frameTime = 0.0f;

    int32_t sceneGraphNodeIndex = 0;

    glm::ivec2 referenceRaycast = glm::ivec2(-1);

    float dragSensitivity = 0.01f;

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
  void preprocessEditorData();
  void showEditor();

  void drawMainMenu();
  void drawSceneGraph();
  void drawScenePanel();
  void drawActorProperties();
  void drawSceneProperties();

  bool drawTreeNode(const std::string& name, const bool isFolder = false);
  void drawVec3Control(const char* label, glm::vec3& vector,
    float speed = 0.01f, const bool locked = false, const char* format = "%.3f");
  void drawFrameInfo();

  void selectSceneGraphItem(const std::string& name, ESceneGraphItemType itemType);
  void selectSceneGraphItem(const int32_t UID);

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