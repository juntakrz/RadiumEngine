#pragma once

#include "common.h"

#define KEY(x) m_inputPairs.at(x)

typedef std::unordered_map<int, std::unordered_map<int, TFuncPtr>>
    TInputBinds;
typedef std::unordered_map<std::string, uint32_t> TInputPairs;

namespace core {

  class MInput {
 private:
  TInputBinds m_inputBinds;  // action id <> function bindings
  TInputPairs m_inputPairs;  // input name <> input key pairs

 private:
  MInput();

  // set up default inputs always used by the engine
  void setDefaultInputPairs();

 public:
  static MInput& get() {
    static MInput _sInstance;
    return _sInstance;
  }

  MInput(const MInput&) = delete;
  MInput& operator=(const MInput&) = delete;

  TResult initialize(GLFWwindow* window);

  void addInputPair(std::string name, std::string key);

  template <typename C>
  void bindFunction(int key, int action, C* owner, void (C::*function)()) {
    auto& keyBind = m_inputBinds[key];
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
}