#include "pch.h"
#include "core/core.h"
#include "core/objects.h"
#include "core/managers/input.h"
#include "core/managers/renderer.h"
#include "core/managers/window.h"
#include "util/util.h"

core::MInput::MInput() {
  RE_LOG(Log, "Creating input manager.");
  setDefaultInputs();
}

void core::MInput::scanInput() {
  // ImGui wants keyboard focus - block GLFW input
  if (ImGui::GetIO().WantCaptureKeyboard) return;

  GLFWwindow* pWindow = core::window.getWindow();

  // Get status for every bound repeated action key and call the proper function
  for (const auto& it : m_inputBinds) {
    int keyState = glfwGetKey(pWindow, it.second);

    if (m_inputFuncsRepeated.contains(it.second)) {
      const auto& keyStateVector = m_inputFuncsRepeated.at(it.second);

      if (keyStateVector[keyState] != nullptr) {
        keyStateVector[keyState]->exec();
      }

    }
  }

  // Process mouse axis bound functions
  for (const auto& it : m_mouseXAxisFunctions) {
    it->exec();
  }

  for (const auto& it : m_mouseYAxisFunctions) {
    it->exec();
  }

  // Reset cursor delta after processing axes
  m_cursorDelta.x = 0.0f;
  m_cursorDelta.y = 0.0f;
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
  glfwSetCursorPosCallback(window, cursorPositionCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);

  bindFunction(GETKEY("devKey"), GLFW_PRESS, this, &MInput::actPressTest);
  bindFunction(GETKEY("devKey"), GLFW_RELEASE, this, &MInput::actReleaseTest);
  bindFunction(GETKEY("controlMode"), GLFW_RELEASE, this, &MInput::actToggleMouseLook);

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

EControlMode core::MInput::getControlMode() {
  return m_controlMode;
}

void core::MInput::setMouseAcceleration(const float value) {
  m_mouseRawAcceleration = 1000.0f * value;
}

const glm::vec2& core::MInput::getMousePosition() {
  return m_cursorPosition;
}

const glm::vec2& core::MInput::getMouseDelta() {
  return m_cursorDelta;
}

const glm::ivec2& core::MInput::getMouseWindowPosition() {
  return m_cursorWindowPosition;
}

void core::MInput::keyEventCallback(GLFWwindow* window, int key, int scancode,
                              int action, int mods) {
  // ImGui wants to capture keyboard input, block GLFW input
  if (ImGui::GetIO().WantCaptureKeyboard) return;

  // Get status for every bound single action key and call the proper function
  if (get().m_inputFuncsSingle.contains(key)) {
    const auto& keyStateVector = get().m_inputFuncsSingle.at(key);

    if (keyStateVector[action] != nullptr) {
      keyStateVector[action]->exec();
    }
  }
}

void core::MInput::cursorPositionCallback(GLFWwindow* window, double x, double y) {
  // Convert to screen space
  MInput& input = get();

  input.m_cursorWindowPosition.x = static_cast<int32_t>(x);
  input.m_cursorWindowPosition.y = static_cast<int32_t>(y);

  float cursorX = static_cast<float>(x / config::renderWidth * 2.0f - 1.0f);
  float cursorY = static_cast<float>(y / config::renderHeight * 2.0f - 1.0f);

  input.m_cursorDelta.x = (cursorX - input.m_cursorPosition.x) * input.m_mouseRawAcceleration;
  input.m_cursorDelta.y = (input.m_cursorPosition.y - cursorY) * input.m_mouseRawAcceleration;

  input.m_cursorPosition.x = cursorX;
  input.m_cursorPosition.y = cursorY;

  //RE_LOG(Log, "Cursor delta X: %1.3f / Y: %1.3f", input.m_cursorDelta.x, input.m_cursorDelta.y);
}

void core::MInput::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  if (config::bDevMode) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
      switch (get().m_controlMode) {
        case EControlMode::Cursor: {
          core::renderer.setRaycastPosition(get().getMouseWindowPosition());
          break;
        }
      }
    }
  }
}

void core::MInput::actPressTest() { RE_LOG(Log, "Test key pressed."); }

void core::MInput::actReleaseTest() { RE_LOG(Log, "Test key released."); }

void core::MInput::actToggleMouseLook() {
  GLFWwindow* pWindow = core::window.getWindow();

  switch (m_controlMode) {
    case EControlMode::Cursor: {
      glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

      if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
      }

      m_controlMode = EControlMode::MouseLook;
      break;
    }
    case EControlMode::MouseLook: {
      glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

      if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
      }

      m_controlMode = EControlMode::Cursor;
      break;
    }
  }
}
