#pragma once

namespace core {

  class MTime {
    //std::thread coreTimerThread;
    std::chrono::steady_clock::time_point initialTimePoint;
    std::chrono::steady_clock::time_point oldTimePoint;
    std::chrono::steady_clock::time_point currentTimePoint;
    float lastDeltaTime = 0.0f;

    MTime();

  public:
    static MTime& get() {
      static MTime _sInstance;
      return _sInstance;
    }

    MTime(const MTime&) = delete;
    MTime& operator=(const MTime&) = delete;

    // ticks timer, should be called at the start of the frame,
    // stores and returns delta time
    float tickTimer();

    // delta time between last two ticks
    float getDeltaTime();

    // time since initial time point (engine initialization)
    float getTimeSinceInitialization();
  };
}