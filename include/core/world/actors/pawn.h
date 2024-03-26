#pragma once

#include "core/world/components/transformcomp.h"
#include "core/world/actors/entity.h"

class APawn : public AEntity {
 public:
  APawn() = default;
  APawn(const uint32_t UID) { m_UID = UID; m_typeId = EActorType::Pawn; addComponent<WTransformComponent>(); }
  virtual ~APawn() override{};
  virtual const EActorType& getTypeId() override { return m_typeId; }
};