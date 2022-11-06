#pragma once

#include "common.h"

class OFuncPtr_Base {
public:
  virtual void exec(){};
  virtual void operator()(){};
};

template<typename C>
class OFuncPtr : public OFuncPtr_Base {
  C* owner = nullptr;
  void (C::*func)() = nullptr;

public:
  //OFuncPtr(){};
  OFuncPtr(C* pOwner, void(C::*pFunc)()) : owner(pOwner), func(pFunc){};

  virtual void exec() override {
    (*this->owner.*this->func)();
  }
  virtual void operator()() override { return (*this->owner.*this->func)(); }
};

//typedef std::unique_ptr<OFuncPtr_Base> funcPtrObject;
typedef OFuncPtr_Base* funcPtrObject;
typedef std::unordered_map<int, std::unordered_map<int, funcPtrObject>> TInputBinds;

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
    //keyBind[action] = std::unique_ptr<OFuncPtr<C>>(owner, function);
    keyBind[action] = new OFuncPtr<C>(owner, function);
  }

  static TInputBinds& binds();

  static void keyEventCallback(GLFWwindow* window, int key, int scancode,
                               int action, int mods);

  void actPressTest();
  void actReleaseTest();
};