#pragma once

namespace core {
class MGUI {
private:
  ImGuiIO* m_pIO = nullptr;

  MGUI();
  void checkVulkanResult(VkResult result) {};

public:
  static MGUI& get() {
    static MGUI _sInstance;
    return _sInstance;
  }

  void initialize();
  void deinitialize();

  ImGuiIO& io();

  void render();
};
}