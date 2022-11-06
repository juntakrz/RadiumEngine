#include "pch.h"
#include "core/managers/minput.h"
#include "util/util.h"

MInput::MInput() { RE_LOG(Log, "Creating input manager."); }

TResult MInput::initialize(GLFWwindow* window) {
  TResult chkResult = RE_OK;

  glfwSetKeyCallback(window, keyEventCallback);

  bindFunction(GLFW_KEY_E, GLFW_PRESS, &actPressTest);
  bindFunction(GLFW_KEY_E, GLFW_RELEASE, &actReleaseTest);

  return chkResult;
}

TResult MInput::bindFunction(int key, int action, TFunc function) {
  if (!function) {
    RE_LOG(Error,
           "Failed to bind function to a key '%d' with action '%d': no "
           "function ptr provided.");

    return RE_ERROR;
  }

  auto& keyBind = inputBinds[key];
  keyBind[action] = function;

  return RE_OK;
}

TResult MInput::testFunction(int key, int action, void* function) {
  TFunc pFunc = (TFunc)function;

  auto& keyBind = inputBinds[key];
  keyBind[action] = pFunc;

  return RE_OK;
}

TInputBinds& MInput::binds() { return get().inputBinds; }

void MInput::keyEventCallback(GLFWwindow* window, int key, int scancode,
                              int action, int mods) {
  using func = void (*)();

  if (binds().find(key) != binds().end()) {
    auto& keyBind = binds()[key];
    if (keyBind.find(action) != keyBind.end()) {
      ((func)keyBind[action])();
    }
  }
}

void MInput::actPressTest() { RE_LOG(Log, "Key pressed."); }

void MInput::actReleaseTest() { RE_LOG(Log, "Key released."); }
