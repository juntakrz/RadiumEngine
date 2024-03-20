#pragma once

#include "core/world/actors/entity.h"

class APawn : public AEntity {
 public:
  APawn() = default;
  APawn(const uint32_t UID) { m_UID = UID; m_typeId = EActorType::Pawn; }
  virtual ~APawn() override{};
};