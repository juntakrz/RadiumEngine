#pragma once

#include "core/world/actors/entity.h"

class APawn : public AEntity {
 protected:
  EActorType m_typeId = EActorType::Pawn;

 public:
  APawn() noexcept {};
  virtual ~APawn() override{};
};