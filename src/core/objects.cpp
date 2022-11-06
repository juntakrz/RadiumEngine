#include "pch.h"
#include "core/objects.h"

std::set<int32_t> RVkQueueFamilyIndices::getAsSet() const {
  if (graphics.empty() || compute.empty() ||
      present.empty()) {
    return { -1 };
  }

  return {graphics[0], compute[0], present[0]};
}
