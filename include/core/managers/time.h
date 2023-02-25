#pragma once

namespace core {

  class MTime {
    //std::thread coreTimerThread;
    double m_initialTimePoint;
    double m_oldTimePoint;
    double m_currentTimePoint;
    float m_lastDeltaTime = 0.0f;

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