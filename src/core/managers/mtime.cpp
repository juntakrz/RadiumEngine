#include "pch.h"
#include "core/managers/mtime.h"

core::MTime::MTime() {
  m_currentTimePoint = glfwGetTime();
  m_initialTimePoint = m_currentTimePoint;
}

float core::MTime::tickTimer() {
  m_oldTimePoint = m_currentTimePoint;
  m_currentTimePoint = glfwGetTime();

  return m_lastDeltaTime = static_cast<float>(m_currentTimePoint - m_oldTimePoint);
}

float core::MTime::getDeltaTime() { return m_lastDeltaTime; }

float core::MTime::getTimeSinceInitialization() {
  m_oldTimePoint = m_currentTimePoint;
  m_currentTimePoint = glfwGetTime();

  return static_cast<float>(m_currentTimePoint - m_initialTimePoint);
}
