#pragma once

#include "core/world/actors/base.h"

class WModel;

class AEntity : public ABase {
 protected:
  EActorType m_typeId = EActorType::Entity;

  WModel* m_pModel = nullptr;
  int32_t m_bindIndex = -1;

 public:
  AEntity() noexcept {};
  virtual ~AEntity() override{};

  virtual void setModel(WModel* pModel);
  virtual WModel* getModel();
  virtual void updateModel();
  virtual void bindToRenderer();
  virtual void unbindFromRenderer();
  int32_t getBindingIndex();
  void setBindingIndex(int32_t bindingIndex);
};