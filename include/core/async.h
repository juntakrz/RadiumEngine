#pragma once

#include "config.h"

class RAsync {
  struct BoundFunction {
    TFuncPtr function;
    std::condition_variable* pConditional;
  };

  std::thread thread;
  std::mutex mutex;
  std::condition_variable conditional;
  std::vector<std::condition_variable*> pBoundConditionals;
  std::vector<BoundFunction> boundFunctions;
  bool cue = false;
  bool execute = true;

  void loop();

 public:
  // example: bindFunction(this, &RClass::method)
  template <typename C>
  void bindFunction(C* owner, void (C::*function)(), std::condition_variable* pOptionalConditional = nullptr) {
    boundFunctions.emplace_back(std::make_unique<OFuncPtr<C>>(owner, function), pOptionalConditional);
  }

  void unbindFunctions() { boundFunctions.clear(); }

  // bind function before calling start()
  void start();

  // immediately stop this thread
  void stop();

  // execute bound function
  void update();
};