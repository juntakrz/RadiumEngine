#include "pch.h"
#include "core/world/actors/abase.h"
#include "core/managers/mref.h"

void MRef::registerActor(const char* name, ABase* pActor, EAType type) {
  if (!actorPointers.try_emplace(name).second) {
    RE_LOG(Error, "Failed to register actor '%s'.", name);
    return;
  }

  actorPointers.at(name).ptr = pActor;
  actorPointers.at(name).type = type;
}
