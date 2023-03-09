#pragma once

// universal method pointer storage code
class OFuncPtr_Base {
 public:
  virtual void exec(){};
  virtual void operator()(){};
};

template <typename C>
class OFuncPtr : public OFuncPtr_Base {
  C* owner = nullptr;
  void (C::*func)() = nullptr;
  void (C::*funcWithParameter)(float) = nullptr;

 public:
  OFuncPtr(C* pOwner, void (C::*pFunc)()) : owner(pOwner), func(pFunc){};

  virtual void exec() override { (*this->owner.*this->func)(); }
  virtual void operator()() { return (*this->owner.*this->func)(); }
};

typedef std::unique_ptr<OFuncPtr_Base> TFuncPtr;

// generic type definitions
typedef unsigned int TResult;  // error result