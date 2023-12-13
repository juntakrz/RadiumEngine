#pragma once

#include "config.h"

class RAsync {
  std::thread thread;
  std::mutex mutex;
  std::condition_variable conditional;
  std::vector<TFuncPtr> boundFunctions;
  bool cue = false;
  bool execute = true;

  void loop();

 public:
  // example: bindFunction(this, &RClass::method)
  template <typename C>
  void bindFunction(C* owner, void (C::*function)()) {
    boundFunctions.emplace_back(std::make_unique<OFuncPtr<C>>(owner, function));
  }

  void unbindFunctions() { boundFunctions.clear(); }

  // bind function before calling start()
  void start();

  // immediately stop this thread
  void stop();

  // execute bound function
  void update();
};