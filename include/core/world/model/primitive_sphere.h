#pragma once

#include "primitive.h"

class WPrimitive_Sphere : public WPrimitive {
 public:
  virtual void create(int arg0 = 16, int arg1 = 0) override;
};