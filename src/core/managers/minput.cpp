#include "pch.h"
#include "core/managers/minput.h"
#include "util/util.h"

core::MInput::MInput() { RE_LOG(Log, "Creating input manager."); }

void core::MInput::setDefaultInputPairs() {
  RE_LOG(Log, "Setting up default input bindings.");
  m_inputPairs["moveForward"] = GLFW_KEY_W;
  m_inputPairs["moveBack"] = GLFW_KEY_S;
  m_inputPairs["moveLeft"] = GLFW_KEY_A;
  m_inputPairs["moveRight"] = GLFW_KEY_D;
  m_inputPairs["moveUp"] = GLFW_KEY_LEFT_SHIFT;
  m_inputPairs["moveDown"] = GLFW_KEY_LEFT_CONTROL;
  m_inputPairs["mouseMode"] = GLFW_KEY_SPACE;
  m_inputPairs["devKey"] = GLFW_KEY_F1;
}

TResult core::MInput::initialize(GLFWwindow* window) {
  TResult chkResult = RE_OK;

  glfwSetKeyCallback(window, keyEventCallback);

  setDefaultInputPairs();

  bindFunction(KEY("devKey"), GLFW_PRESS, this, &MInput::actPressTest);
  bindFunction(KEY("devKey"), GLFW_RELEASE, this, &MInput::actReleaseTest);

  return chkResult;
}

void core::MInput::addInputPair(std::string name, std::string key) {}

TInputBinds& core::MInput::binds() { return get().m_inputBinds; }

void core::MInput::keyEventCallback(GLFWwindow* window, int key, int scancode,
                              int action, int mods) {
  using func = void (*)();

  if (binds().find(key) != binds().end()) {
    auto& keyBind = binds()[key];
    if (keyBind.find(action) != keyBind.end()) {
      keyBind[action]->exec();
    }
  }
}

void core::MInput::actPressTest() { RE_LOG(Log, "Test key pressed."); }

void core::MInput::actReleaseTest() { RE_LOG(Log, "Test key released."); }