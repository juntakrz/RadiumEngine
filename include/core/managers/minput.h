#pragma once

#include "common.h"

class MInput {
  MInput();

  TInputBinds inputBinds;

public:
  static MInput& get() {
    static MInput _sInstance;
    return _sInstance;
  }

  MInput(const MInput&) = delete;
  MInput& operator=(const MInput&) = delete;

  TResult initialize(GLFWwindow* window);

  template <typename C>
  void bindFunction(int key, int action, C* owner, void (C::*function)()) {
    auto& keyBind = inputBinds[key];
    keyBind[action] = std::make_unique<OFuncPtr<C>>(owner, function);
  }

  static TInputBinds& binds();

  static void keyEventCallback(GLFWwindow* window, int key, int scancode,
                               int action, int mods);

  void actPressTest();
  void actReleaseTest();
};