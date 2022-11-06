#pragma once

// universal method pointer storage code
class OFuncPtr_Base {
 public:
  virtual void exec(){};
};

template <typename C>
class OFuncPtr : public OFuncPtr_Base {
  C* owner = nullptr;
  void (C::*func)() = nullptr;

 public:
  OFuncPtr(C* pOwner, void (C::*pFunc)()) : owner(pOwner), func(pFunc){};

  virtual void exec() override { (*this->owner.*this->func)(); }
};

typedef std::unique_ptr<OFuncPtr_Base> funcPtrObject;

// used by the input manager class (MInput)
typedef std::unordered_map<int, std::unordered_map<int, funcPtrObject>>
    TInputBinds;

// generic type definitions
typedef unsigned int TResult;  // error result