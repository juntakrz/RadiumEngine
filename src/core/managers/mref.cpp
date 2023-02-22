#include "pch.h"
#include "core/objects.h"
#include "core/world/actors/acamera.h"
#include "core/managers/mref.h"

core::MRef::MRef() { RE_LOG(Log, "Created reference manager."); }

void core::MRef::registerActor(const char* name, ABase* pActor, EAType type) {
  if (!actorPointers.try_emplace(name).second) {
    RE_LOG(Error, "Failed to register actor '%s'.", name);
    return;
  }

  actorPointers.at(name).ptr = pActor;
  actorPointers.at(name).type = type;
}
