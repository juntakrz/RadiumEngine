#include "pch.h"
#include "util/util.h"
#include "core/world/actors/pawn.h"

void APawn::setModel(WModel* pModel) { m_pModel = pModel; }

WModel* APawn::getModel() { return m_pModel; }
