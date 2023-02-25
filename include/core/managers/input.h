#pragma once

#include "common.h"

#define GETKEY(x) core::MInput::get().bindingToKey(x)

// stores function calls for a given input
// structure is: { key , funcPtr[keyState] }
typedef std::unordered_map<int, std::array<TFuncPtr, 3>>
    TInputFuncs;

// stores string aliases for GLFW keys
typedef std::unordered_map<std::string, uint32_t> TInputAliases;

// stores control - GLFW key bind
typedef std::unordered_map<std::string, uint32_t> TInputBinds;

namespace core {

class MInput {
 private:
  // bindings that should be triggered every frame / repeated action
  // (example: while they key is pressed or released, e.g. movement)
  TInputFuncs m_inputFuncsRepeated;

  // bindings that should be triggered only once / single action
  // (example: once when they key is pressed on released)
  TInputFuncs m_inputFuncsSingle;

  // string aliases for GLFW key codes (e.g. "W" for GLFW_KEY_W)
  TInputAliases m_inputAliases;

  // GLFW key codes and function names bound to them
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

  // scans input for all bound inputs, should be called every frame
  void scanInput();

  uint32_t bindingToKey(const char* bindingName);

  void setInputBinding(std::string name, std::string key);

  template <typename C>
  void bindFunction(int key, int action, C* owner, void (C::*function)(),
                    bool bRepeated = false) {
    auto& keyBind =
        (bRepeated) ? m_inputFuncsRepeated[key] : m_inputFuncsSingle[key];
    keyBind[action] = std::make_unique<OFuncPtr<C>>(owner, function);
  }

  // get either repeated or single input function bindings
  static TInputFuncs& getBindings(bool bRepeated);

  static void keyEventCallback(GLFWwindow* window, int key, int scancode,
                               int action, int mods);
};
}  // namespace core