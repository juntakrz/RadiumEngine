#pragma once

#include "common.h"

class MModel {
  MModel();

 public:
  static MModel& get() {
    static MModel _sInstance;
    return _sInstance;
  }

  MModel(const MModel&) = delete;
  MModel& operator=(const MModel&) = delete;
};