#include "pch.h"
#include "core/world/actors/base.h"
#include "core/world/components/component.h"

void WComponent::invalidComponentErrorMessage() {
  RE_LOG(Error, "Invalid component event type for '%s'.", pOwner->getName().c_str());
}
