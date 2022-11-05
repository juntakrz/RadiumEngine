#include "pch.h"
#include "core/managers/minput.h"
#include "util/util.h"

MInput::MInput() { RE_LOG(Log, "Creating input manager."); }

TResult MInput::initialize(GLFWwindow* window) {
  TResult chkResult = RE_OK;

  glfwSetKeyCallback(window, keyEventCallback);

  bindFunction(GLFW_KEY_E, GLFW_PRESS, this, &MInput::actPressTest);
  bindFunction(GLFW_KEY_E, GLFW_RELEASE, this, &MInput::actReleaseTest);

  return chkResult;
}

TInputBinds& MInput::binds() { return get().inputBinds; }

void MInput::keyEventCallback(GLFWwindow* window, int key, int scancode,
                              int action, int mods) {
  using func = void (*)();

  if (binds().find(key) != binds().end()) {
    auto& keyBind = binds()[key];
    if (keyBind.find(action) != keyBind.end()) {
      keyBind[action]->exec();
    }
  }
}

void MInput::actPressTest() { RE_LOG(Log, "Key pressed."); }

void MInput::actReleaseTest() { RE_LOG(Log, "Key released."); }