#pragma once

#include "primitive.h"

class WPrimitive_Custom : public WPrimitive {
 public:
  bool isCustom = true;

 public:
  WPrimitive_Custom(RPrimitiveInfo* pCreateInfo);
};