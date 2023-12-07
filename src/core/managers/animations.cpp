#include "pch.h"
#include "util/util.h"
#include "core/managers/animations.h"

WAnimation* core::MAnimations::createAnimation(const std::string& inName,
                                       const int32_t inFramerate) {
  if (m_animations.find(inName) == m_animations.end()) {
    m_animations[inName] = std::make_unique<WAnimation>(inName, inFramerate);
  }

  return m_animations.at(inName).get();
}

void core::MAnimations::removeAnimation(const std::string& inName) {
  if (m_animations.find(inName) == m_animations.end()) {
    RE_LOG(Warning, "Failed to remove animation '%s'. It does not exist.",
           inName.c_str());
    return;
  }

  // m_animations.at(inName).release();
  m_animations.erase(inName);
}

WAnimation* core::MAnimations::getAnimation(const std::string& inName) {
  if (m_animations.find(inName) == m_animations.end()) {
    RE_LOG(Error, "Failed to get animation '%s'. It does not exist.",
           inName.c_str());
    return nullptr;
  }

  return m_animations.at(inName).get();
}

void core::MAnimations::clearAllAnimations() { m_animations.clear(); }