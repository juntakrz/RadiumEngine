#pragma once

#include "core/objects.h"

class ABase;
struct ComponentEvent;
class ComponentEventSystem;

struct WComponent {
  EComponentType typeId = EComponentType::Base;
  ABase* pOwner = nullptr;
  ComponentEventSystem* pEvents = nullptr;

  WComponent(ABase* pActor = nullptr) : pOwner(pActor) {};

  virtual void onEvent(const ComponentEvent& newEvent) {};
  virtual void drawComponentUI() { ImGui::Text("Error. Base WComponent is a parent template and should never be used as is."); };
  virtual void update() {};
};