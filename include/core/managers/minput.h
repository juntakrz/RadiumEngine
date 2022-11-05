#pragma once

#include "common.h"

typedef std::unordered_map<int, std::unordered_map<int, TFuncPtr>>
    TInputBinds;

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

  template <typename C>
  void bindFunction(const char* actionName, int action, C* owner,
                    void (C::*function)()) {
    // looks up a key code from the std::unordered_map<std::string, int>
    // by 'actionName' and calls key code based bindFunction with it
    // can be useful for separately configured named key actions
  }

  static TInputBinds& binds();

  static void keyEventCallback(GLFWwindow* window, int key, int scancode,
                               int action, int mods);

  void actPressTest();
  void actReleaseTest();
};