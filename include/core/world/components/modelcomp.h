#pragma once

#include "component.h"

class WModel;

struct WModelComponent : public WComponent {
  struct {
    WModel* pModel = nullptr;
  } data;

  WModelComponent(ABase* pActor) { typeId = EComponentType::Model; pOwner = pActor; }

  WModel* getModel();
  void setModel(WModel* pModel);

  void update() override;
  void drawComponentUI() override;
};