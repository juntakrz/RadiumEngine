#include "pch.h"
#include "core/managers/mtime.h"

core::MTime::MTime() {
  currentTimePoint = std::chrono::high_resolution_clock::now();
  initialTimePoint = currentTimePoint;
}

float core::MTime::tickTimer() {
  oldTimePoint = currentTimePoint;
  currentTimePoint = std::chrono::high_resolution_clock::now();
  return lastDeltaTime =
             std::chrono::duration<float, std::chrono::seconds::period>(
                 currentTimePoint - oldTimePoint)
                 .count();
}

float core::MTime::getDeltaTime() { return lastDeltaTime; }

float core::MTime::getTimeSinceInitialization() {
  return std::chrono::duration<float, std::chrono::seconds::period>(
             currentTimePoint - initialTimePoint)
      .count();
}
