#include "pch.h"
#include "core/world/actors/base.h"
#include "core/world/components/component.h"

void WComponent::invalidComponentErrorMessage() {
  RE_LOG(Error, "Invalid component event type for '%s'.", pOwner->getName().c_str());
}

void WComponent::drawComponentUI(const uint32_t index) {
  ImGui::Text("Error. Base WComponent is a parent template and should never be used as is.");
}
