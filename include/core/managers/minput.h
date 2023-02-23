#pragma once

#include "common.h"

#define GETKEY(x) core::MInput::get().bindingToKey(x)

// stores function calls for a given input
typedef std::unordered_map<int, std::unordered_map<int, TFuncPtr>>
    TInputFuncs;

// stores string aliases for GLFW keys
typedef std::unordered_map<std::string, uint32_t> TInputAliases;

// stores control - GLFW key bind
typedef std::unordered_map<std::string, uint32_t> TInputBinds;

namespace core {

class MInput {
 private:
  TInputFuncs m_inputFuncs;
  TInputAliases m_inputAliases;
  TInputBinds m_inputBinds;

 private:
  MInput();

  void actPressTest();
  void actReleaseTest();

  // set up default inputs always used by the engine
  void setDefaultInputs();

 public:
  static MInput& get() {
    static MInput _sInstance;
    return _sInstance;
  }

  MInput(const MInput&) = delete;
  MInput& operator=(const MInput&) = delete;

  TResult initialize(GLFWwindow* window);

  uint32_t bindingToKey(const char* bindingName);

  void setInputBinding(std::string name, std::string key);

  template <typename C>
  void bindFunction(int key, int action, C* owner, void (C::*function)()) {
    auto& keyBind = m_inputFuncs[key];
    keyBind[action] = std::make_unique<OFuncPtr<C>>(owner, function);
  }

  static TInputFuncs& binds();

  static void keyEventCallback(GLFWwindow* window, int key, int scancode,
                               int action, int mods);
};
}  // namespace core