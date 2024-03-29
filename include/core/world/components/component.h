#pragma once

#include "core/objects.h"

class ABase;
struct ComponentEvent;
class ComponentEventSystem;

struct WComponent {
  EComponentType typeId = EComponentType::Base;
  EAttachmentMode attachmentMode = EAttachmentMode::None;
  ABase* pOwner = nullptr;
  ABase* pTarget = nullptr;
  ComponentEventSystem* pEvents = nullptr;

  WComponent(ABase* pActor = nullptr) : pOwner(pActor) {};

  ABase* getOwner() { return pOwner; }

  void invalidComponentErrorMessage();

  virtual void onAttachmentModeChanged(ABase*, EAttachmentMode) {};
  virtual void onOwnerControlled() {};
  virtual void onOwnerFreed() {};
  virtual void onOwnerUpdated() {};
  virtual void drawComponentUI() { ImGui::Text("Error. Base WComponent is a parent template and should never be used as is."); };
  virtual void update() {};
};