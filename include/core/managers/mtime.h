#pragma once

namespace core {

  class MTime {
    std::thread coreTimer;

    MTime();

  public:
    static MTime& get() {
      static MTime _sInstance;
      return _sInstance;
    }

    MTime(const MTime&) = delete;
    MTime& operator=(const MTime&) = delete;

    void startCoreTimer();
    void stopCoreTimer();
  };
}