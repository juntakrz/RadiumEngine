#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/model/model.h"
#include "core/world/actors/entity.h"

void AEntity::setModel(WModel* pModel) { m_pModel = pModel; }

WModel* AEntity::getModel() { return m_pModel; }

void AEntity::updateModel() {
  if (m_pModel) {
    m_pModel->update(getTransformationMatrix());
  }
}

void AEntity::bindToRenderer() {
  if (m_bindIndex < 0) {
    RE_LOG(Warning, "Entity \"%s\" is already bound to renderer.", m_name);
  }

  m_bindIndex = (int32_t)core::renderer.bindEntity(this);
}

void AEntity::unbindFromRenderer() {
  core::renderer.unbindEntity(m_bindIndex);
  m_bindIndex = -1;
}

int32_t AEntity::getBindingIndex() { return m_bindIndex; }

void AEntity::setBindingIndex(int32_t bindingIndex) {
  m_bindIndex = bindingIndex;
}
