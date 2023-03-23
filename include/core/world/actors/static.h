#pragma once

#include "core/world/actors/entity.h"

class AStatic : public AEntity {
 protected:
  EActorType m_typeId = EActorType::Static;

 public:
  AStatic() noexcept {};
  virtual ~AStatic() override{};
};