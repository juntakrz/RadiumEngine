#include "pch.h"
#include "core/managers/minput.h"
#include "util/util.h"

core::MInput::MInput() {
  RE_LOG(Log, "Creating input manager.");
  setDefaultInputs();
}

uint32_t core::MInput::bindingToKey(const char* bindingName) { 
  if (m_inputBinds.find(bindingName) != m_inputBinds.end()) {
    return m_inputBinds.at(bindingName);
  }

  RE_LOG(Error, "Failed to find binding '%s'.", bindingName);
  return -1;
}

TResult core::MInput::initialize(GLFWwindow* window) {
  TResult chkResult = RE_OK;

  glfwSetKeyCallback(window, keyEventCallback);

  bindFunction(GETKEY("devKey"), GLFW_PRESS, this, &MInput::actPressTest);
  bindFunction(GETKEY("devKey"), GLFW_RELEASE, this, &MInput::actReleaseTest);

  return chkResult;
}

void core::MInput::setInputBinding(std::string name, std::string key) {
  if (m_inputAliases.find(key) != m_inputAliases.end()) {
    m_inputBinds[name] = m_inputAliases[key];

    return;
  }

  RE_LOG(Error, "%s: key id for '%s' was not resolved, won't bind to '%s'.",
         __FUNCTION__, key.c_str(), name.c_str());
}

TInputFuncs& core::MInput::binds() { return get().m_inputFuncs; }

void core::MInput::keyEventCallback(GLFWwindow* window, int key, int scancode,
                              int action, int mods) {
  //using func = void (*)();

  if (binds().find(key) != binds().end()) {
    auto& keyBind = binds()[key];
    if (keyBind.find(action) != keyBind.end()) {
      keyBind[action]->exec();
    }
  }
}

void core::MInput::actPressTest() { RE_LOG(Log, "Test key pressed."); }

void core::MInput::actReleaseTest() { RE_LOG(Log, "Test key released."); }