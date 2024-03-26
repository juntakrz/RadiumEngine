#include "pch.h"
#include "core/model/model.h"
#include "core/world/components/modelcomp.h"

WModel* WModelComponent::getModel() {
  return data.pModel;
}

void WModelComponent::setModel(WModel* pModel) {
  data.pModel = pModel;
}

void WModelComponent::update() {
}

void WModelComponent::drawComponentUI() {
}
