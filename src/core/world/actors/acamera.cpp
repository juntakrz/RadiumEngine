#include "pch.h"
#include "core/objects.h"
#include "core/world/actors/acamera.h"

void ACamera::setPerspective(float FOV, float aspectRatio, float nearZ,
                               float farZ) noexcept {
  m_ModelViewProj.matProjection =
      glm::perspective(FOV, aspectRatio, nearZ, farZ);
}
