#pragma once

#include "core/world/components/transformcomp.h"
#include "core/world/actors/entity.h"

class AStatic : public AEntity {
 public:
  AStatic() = default;
  AStatic(const uint32_t UID) { m_UID = UID; m_typeId = EActorType::Static; addComponent<WTransformComponent>(); }
  virtual ~AStatic() override{};
  virtual const EActorType& getTypeId() override { return m_typeId; }
};