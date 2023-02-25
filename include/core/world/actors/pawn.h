#pragma once

#include "core/world/actors/base.h"

class WModel;

class APawn : public ABase {
 protected:
  EActorType m_typeId = EActorType::Pawn;

  WModel* m_pModel = nullptr;

 public:
  APawn() noexcept {};
  virtual ~APawn() override{};
};