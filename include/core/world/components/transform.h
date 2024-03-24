#pragma once

#include "component.h"

class WTransformComponent : public WComponent {
public:
  void showUIElement(ABase* pActor) override;
};