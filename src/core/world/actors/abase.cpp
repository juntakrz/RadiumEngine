#include "pch.h"
#include "core/world/actors/abase.h"
#include "util/math.h"

glm::mat4& ABase::GetTransform() noexcept { return m_matMain; }

ABase::TransformData& ABase::GetTransformData() noexcept { return transform; }

void ABase::setPos(float x, float y, float z) noexcept {
  transform.translation.x = x;
  transform.translation.y = y;
  transform.translation.z = z;

  transform.initial.translation = transform.translation;
}

void ABase::setPos(const glm::vec3& pos) noexcept {
  transform.translation = pos;
  transform.initial.translation = transform.translation;
}

glm::vec3& ABase::getPos() noexcept { return transform.translation; }

void ABase::addPos(float x, float y, float z) noexcept {
  transform.translation += glm::vec3(x, y, z);
}

void ABase::addPos(const glm::vec3& delta) noexcept {
  transform.translation += delta;
}

void ABase::setRotation(float x, float y, float z) noexcept {
  math::wrapAnglesGLM(transform.rotation = glm::vec3(x, y, z));
  transform.initial.rotation = transform.rotation;
}

void ABase::setRotation(const glm::vec3& rotation) noexcept {
  math::wrapAnglesGLM(transform.rotation = rotation);
  transform.initial.rotation = transform.rotation;
}

glm::vec3& ABase::getRotation() noexcept { return transform.rotation; }

void ABase::addRotation(float x, float y, float z) noexcept {
  math::wrapAnglesGLM(transform.rotation += glm::vec3(x, y, z));
}

void ABase::addRotation(const glm::vec3& delta) noexcept {
  math::wrapAnglesGLM(transform.rotation += delta);
}

void ABase::setScale(float x, float y, float z) noexcept {
  transform.scaling = glm::vec3(x, y, z);
  transform.initial.scaling = transform.scaling;
}

void ABase::setScale(const glm::vec3& scale) noexcept {
  transform.scaling = scale;
  transform.initial.scaling = transform.scaling;
}

glm::vec3& ABase::getScale() noexcept { return transform.scaling; }

void ABase::addScale(float x, float y, float z) noexcept {
  transform.scaling *= glm::vec3(x, y, z);
}

void ABase::addScale(const glm::vec3& delta) noexcept {
  transform.scaling *= delta;
}

uint8_t ABase::TypeId() { return 0; }