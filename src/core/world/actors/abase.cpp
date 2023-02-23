#include "pch.h"
#include "core/world/actors/abase.h"
#include "core/world/actors/acamera.h"
#include "util/math.h"
#include "util/util.h"

glm::mat4& ABase::GetTransform() noexcept { return m_mainTransformationMatrix; }

ABase::TransformData& ABase::GetTransformData() noexcept {
  return m_transformData;
}

void ABase::setPos(float x, float y, float z) noexcept {
  m_transformData.translation.x = x;
  m_transformData.translation.y = y;
  m_transformData.translation.z = z;

  m_transformData.initial.translation = m_transformData.translation;
}

void ABase::setPos(const glm::vec3& pos) noexcept {
  m_transformData.translation = pos;
  m_transformData.initial.translation = m_transformData.translation;
}

glm::vec3& ABase::getPos() noexcept { return m_transformData.translation; }

void ABase::translate(float x, float y, float z) noexcept {
  m_transformData.translation += glm::vec3(x, y, z);
}

void ABase::translate(const glm::vec3& delta) noexcept {
  m_transformData.translation += delta;
}

void ABase::setRotation(float x, float y, float z) noexcept {
  math::wrapAnglesGLM(m_transformData.rotation = glm::vec3(x, y, z));
  m_transformData.initial.rotation = m_transformData.rotation;
}

void ABase::setRotation(const glm::vec3& rotation) noexcept {
  math::wrapAnglesGLM(m_transformData.rotation = rotation);
  m_transformData.initial.rotation = m_transformData.rotation;
}

glm::vec3& ABase::getRotation() noexcept { return m_transformData.rotation; }

void ABase::rotate(float x, float y, float z) noexcept {
  math::wrapAnglesGLM(m_transformData.rotation += glm::vec3(x, y, z));
}

void ABase::rotate(const glm::vec3& delta) noexcept {
  math::wrapAnglesGLM(m_transformData.rotation += delta);
}

void ABase::setScale(float x, float y, float z) noexcept {
  m_transformData.scaling = glm::vec3(x, y, z);
  m_transformData.initial.scaling = m_transformData.scaling;
}

void ABase::setScale(const glm::vec3& scale) noexcept {
  m_transformData.scaling = scale;
  m_transformData.initial.scaling = m_transformData.scaling;
}

glm::vec3& ABase::getScale() noexcept { return m_transformData.scaling; }

void ABase::scale(float x, float y, float z) noexcept {
  m_transformData.scaling *= glm::vec3(x, y, z);
}

void ABase::scale(const glm::vec3& delta) noexcept {
  m_transformData.scaling *= delta;
}

const uint32_t ABase::typeId() { return m_typeId; }