#include "pch.h"
#include "core/core.h"
#include "core/managers/minput.h"
#include "core/managers/mwindow.h"
#include "util/util.h"

core::MInput::MInput() {
  RE_LOG(Log, "Creating input manager.");
  setDefaultInputs();
}

void core::MInput::scanInput() {
  GLFWwindow* pWindow = core::window.getWindow();

  // get status for every bound repeated action key and call the proper function
  for (const auto& it : m_inputBinds) {
    int keyState = glfwGetKey(pWindow, it.second);

    if (m_inputFuncsRepeated.contains(it.second)) {
      const auto& keyStateVector = m_inputFuncsRepeated.at(it.second);

      if (keyStateVector[keyState] != nullptr) {
        keyStateVector[keyState]->exec();
      }

    }
  }
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

TInputFuncs& core::MInput::getBindings(bool bRepeated) {
  return (bRepeated) ? get().m_inputFuncsRepeated : get().m_inputFuncsSingle;
}

void core::MInput::keyEventCallback(GLFWwindow* window, int key, int scancode,
                              int action, int mods) {

  // get status for every bound single action key and call the proper function
  if (get().m_inputFuncsSingle.contains(key)) {
    const auto& keyStateVector = get().m_inputFuncsSingle.at(key);

    if (keyStateVector[action] != nullptr) {
      keyStateVector[action]->exec();
    }
  }
}

void core::MInput::actPressTest() { RE_LOG(Log, "Test key pressed."); }

void core::MInput::actReleaseTest() { RE_LOG(Log, "Test key released."); }