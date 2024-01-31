#include "pch.h"
#include "core/objects.h"
#include "core/world/actors/camera.h"
#include "core/managers/ref.h"

core::MRef::MRef() { RE_LOG(Log, "Created reference manager."); }

void core::MRef::registerActor(const char* name, ABase* pActor) {
  if (!m_actorPointers.try_emplace(name).second) {
    RE_LOG(Error, "Failed to register actor '%s'.", name);
    return;
  }

  m_actorPointers.at(name) = pActor;
}

ABase* core::MRef::getActor(const char* name) {
  return m_actorPointers.at(name);
}
