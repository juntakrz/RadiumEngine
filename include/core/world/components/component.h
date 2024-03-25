#pragma once

#include "core/objects.h"

class ABase;

struct WComponent {
  EComponentType typeId = EComponentType::Base;
  ABase* pOwner = nullptr;

  WComponent(ABase* pActor = nullptr) : pOwner(pActor) {};

  virtual void showUIElement() { ImGui::Text("Error. Base WComponent is a parent template and should never be used as is."); };
  virtual void update() {};
};