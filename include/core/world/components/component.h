#pragma once

#include "core/objects.h"

class WActor;
struct ComponentEvent;
class ComponentEventSystem;

struct WComponent {
  EComponentType typeId = EComponentType::Base;
  EAttachmentMode attachmentMode = EAttachmentMode::None;
  WActor* pOwner = nullptr;
  WActor* pTarget = nullptr;
  ComponentEventSystem* pEvents = nullptr;

  WComponent(WActor* pActor = nullptr) : pOwner(pActor) {};
  virtual ~WComponent() {};

  WActor* getOwner() { return pOwner; }

  void invalidComponentErrorMessage();

  virtual void onAttachmentModeChanged(WActor*, EAttachmentMode) {};
  virtual void onOwnerControlled() {};
  virtual void onOwnerFreed() {};
  virtual void onOwnerUpdated() {};
  virtual void onCreated() {};
  virtual void drawComponentUI(const uint32_t index);
  virtual void update() {};
};