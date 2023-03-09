#pragma once

#include "common.h"

namespace core {

  class MWindow {
  private:
    std::unique_ptr<GLFWwindow, dtorWindow> pWindow;

    MWindow();
    ~MWindow() {
      if (pWindow) {
        glfwDestroyWindow(pWindow.get());
        pWindow.reset();
      }
    }

  public:
    static MWindow& get() {
      static MWindow _sInstance;
      return _sInstance;
    }
    MWindow(const MWindow&) = delete;
    MWindow& operator=(const MWindow&) = delete;

    // create window with appropriate parameters
    TResult createWindow(uint32_t width, uint32_t height, const char* title,
                         GLFWmonitor* pMonitor = nullptr,
                         GLFWwindow* pShare = nullptr);

    // destroy provided window, destroy main one if no pointer is provided
    TResult destroyWindow(GLFWwindow* pOtherWindow = nullptr);

    // get ptr to stored window
    GLFWwindow* getWindow();
  };

  void framebufferResizeCallback(GLFWwindow* window, int width, int height);
}