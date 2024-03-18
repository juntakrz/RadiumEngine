#include "pch.h"
#include "util/util.h"
#include "core/async.h"

void RAsync::loop() {
  while (execute) {
    std::unique_lock<std::mutex> lock(mutex);
    conditional.wait(lock, [this]() { return cue || !execute; });

    if (!execute) {
      break;
    }

    for (const auto& functionStruct : boundFunctions) {
      functionStruct.function->exec();

      if (functionStruct.pConditional) {
        functionStruct.pConditional->notify_all();
      }
    }

    cue = false;
  }
}

void RAsync::start() {
  if (boundFunctions.empty()) {
    RE_LOG(Error, "Can't start async object, no function is bound.");
    return;
  }

  cue = true;
  thread = std::thread(&RAsync::loop, this);
}

void RAsync::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex);
    execute = false;
  }

  conditional.notify_all();

  if (thread.joinable()) {
    thread.join();
  }
}

void RAsync::update() {
  std::unique_lock<std::mutex> lock(mutex);
  cue = true;
  conditional.notify_all();
}