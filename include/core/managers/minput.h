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
  TResult bindFunction(int key, int action, TFunc function);
  TResult testFunction(int key, int action, void* function);

  static TInputBinds& binds();

  static void keyEventCallback(GLFWwindow* window, int key, int scancode,
                               int action, int mods);

  static void actPressTest();
  static void actReleaseTest();
};