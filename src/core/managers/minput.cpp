#include "pch.h"
#include "core/managers/minput.h"
#include "util/util.h"

core::MInput::MInput() { RE_LOG(Log, "Creating input manager."); }

TResult core::MInput::initialize(GLFWwindow* window) {
  TResult chkResult = RE_OK;

  glfwSetKeyCallback(window, keyEventCallback);

  setDefaultInputs();

  bindFunction(KEY("devKey"), GLFW_PRESS, this, &MInput::actPressTest);
  bindFunction(KEY("devKey"), GLFW_RELEASE, this, &MInput::actReleaseTest);

  return chkResult;
}

void core::MInput::addInputBinding(std::string name, std::string key) {
  if (m_inputAliases.find(key) != m_inputAliases.end()) {
    m_inputBinds[name] = m_inputAliases[key];

    return;
  }

  RE_LOG(Error, "%s: key id for '%s' was not resolved, won't bind to '%s'.",
         __FUNCTION__, key, name);
}

TInputFuncs& core::MInput::binds() { return get().m_inputFuncs; }

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