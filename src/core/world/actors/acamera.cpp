#include "pch.h"
#include "core/objects.h"
#include "core/world/actors/acamera.h"

void ACamera::setPerspective(float FOV, float aspectRatio, float nearZ,
                               float farZ) noexcept {
  m_projection = glm::perspective(FOV, aspectRatio, nearZ, farZ);
}

glm::mat4& ACamera::view() {
  // TODO: insert return statement here
}

void ACamera::setUpVector(glm::vec4 upVector) { m_vecUp = upVector; }

void ACamera::setUpVector(float x, float y, float z) {
  m_vecUp = {x, y, z};
}
