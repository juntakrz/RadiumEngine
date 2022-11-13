#include "pch.h"
#include "core/world/actors/abase.h"
#include "util/math.h"

glm::mat4& ABase::GetTransform() noexcept { return m_matMain; }

ABase::TransformData& ABase::GetTransformData() noexcept { return transform; }

void ABase::SetPos(float x, float y, float z) noexcept {
  transform.translation.x = x;
  transform.translation.y = y;
  transform.translation.z = z;

  transform.initial.translation = transform.translation;
}

void ABase::SetPos(const glm::vec3& pos) noexcept {
  transform.translation = pos;
  transform.initial.translation = transform.translation;
}

glm::vec3& ABase::GetPos() noexcept { return transform.translation; }

void ABase::Move(float x, float y, float z) noexcept {
  transform.translation.x += x;
  transform.translation.y += y;
  transform.translation.z += z;
}

void ABase::Move(const glm::vec3& delta) noexcept {
  transform.translation.x += delta.x;
  transform.translation.y += delta.y;
  transform.translation.z += delta.z;
}

void ABase::SetRotation(float x, float y, float z) noexcept {
  transform.rotation.x = math::wrapAngle(x);
  transform.rotation.y = math::wrapAngle(y);
  transform.rotation.z = math::wrapAngle(-z);

  transform.initial.rotation = transform.rotation;
}

void ABase::SetRotation(const glm::vec3& rotation) noexcept {
  transform.rotation = {math::wrapAngle(rotation.x),
                        math::wrapAngle(rotation.y),
                        math::wrapAngle(-rotation.z)};
  transform.initial.rotation = transform.rotation;
}

glm::vec3& ABase::GetRotation() noexcept { return transform.rotation; }

void ABase::Rotate(float x, float y, float z) noexcept {
  transform.rotation.x = math::wrapAngle(transform.rotation.x + x);
  transform.rotation.y = math::wrapAngle(transform.rotation.y + y);
  transform.rotation.z = math::wrapAngle(transform.rotation.z - z);
}

void ABase::Rotate(const glm::vec3& delta) noexcept {
  transform.rotation.x = math::wrapAngle(transform.rotation.x + delta.x);
  transform.rotation.y = math::wrapAngle(transform.rotation.y + delta.y);
  transform.rotation.z = math::wrapAngle(transform.rotation.z - delta.z);
}

void ABase::SetScale(float x, float y, float z) noexcept {
  transform.scaling.x = x;
  transform.scaling.y = y;
  transform.scaling.z = z;

  transform.initial.scaling = transform.scaling;
}

void ABase::SetScale(const glm::vec3& scale) noexcept {
  transform.scaling = scale;
  transform.initial.scaling = transform.scaling;
}

glm::vec3& ABase::GetScale() noexcept { return transform.scaling; }

void ABase::Scale(float x, float y, float z) noexcept {
  transform.scaling.x *= x;
  transform.scaling.y *= y;
  transform.scaling.z *= z;
}

void ABase::Scale(const glm::vec3& delta) noexcept {
  transform.scaling *= delta;
}

uint8_t ABase::TypeId() { return 0; }