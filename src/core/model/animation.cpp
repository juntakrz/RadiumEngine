#include "pch.h"
#include "util/util.h"
#include "core/model/animation.h"

const std::string& WAnimation::getName() { return m_name; }

void WAnimation::createEntry(const std::string& inJointName) {
  if (m_data.find(inJointName) == m_data.end()) {
    m_data.emplace(inJointName, std::vector<glm::mat4>());
    RE_LOG(Log, "Created entry '%s' for animation '%s'.", inJointName.c_str(),
           m_name.c_str());
  } else {
    RE_LOG(Warning,
           "Failed to create entry '%s' for animation '%s'. A similar entry is "
           "already present.",
           inJointName.c_str(), m_name.c_str());
  }
}

void WAnimation::removeEntry(const std::string& inJointName) {
  if (m_data.find(inJointName) == m_data.end()) {
    RE_LOG(Warning,
           "Failed to remove entry '%s' for animation '%s'. The entry was not "
           "found.",
           inJointName.c_str(), m_name.c_str());
  } else {
    m_data.erase(inJointName);
    RE_LOG(Log, "Removed entry '%s' for animation '%s'.", inJointName.c_str(),
           m_name.c_str());
  }
}

void WAnimation::addTransformationMatrix(const std::string& inJointName,
                                        const glm::mat4& inMatrix) {
  if (m_data.find(inJointName) == m_data.end()) {
    RE_LOG(Warning,
           "Failed to add transformation matrix '%s' to animation '%s'. The "
           "requested entry was not found.",
           inJointName.c_str(), m_name.c_str());
  } else {
    m_data.at(inJointName).emplace_back(inMatrix);
  }
}

const glm::mat4& WAnimation::getTransformationMatrix(
    const std::string& inJointName, const int32_t inSampleNumber) {
  if (m_data.find(inJointName) == m_data.end()) {
    RE_LOG(
        Warning,
        "Failed to get transformation matrix for '%s' of animation '%s'. The "
        "requested entry was not found.",
        inJointName.c_str(), m_name.c_str());
    return glm::mat4(1.0f);
  }

  if (inSampleNumber < 0)
    return m_data.at(inJointName)[0];
  else if (inSampleNumber > m_totalFrameSamples)
    return m_data.at(inJointName)[m_totalFrameSamples];

  return m_data.at(inJointName)[inSampleNumber];
}

const glm::mat4& WAnimation::getTransformationMatrixFast(
    const std::string& inJointName, const int32_t inSampleNumber) {
  if (inSampleNumber > m_totalFrameSamples) {
    return m_data.at(inJointName)[m_totalFrameSamples];
  }

  return m_data.at(inJointName)[inSampleNumber];
}

void WAnimation::setupAnimation(const std::string& inName,
                               const float inLengthInSeconds,
                               const int32_t inFramerate) {
  m_name = inName;
  m_framerate = inFramerate;
  m_length = inLengthInSeconds;
}

void WAnimation::clear() {
  m_data.clear();
  m_length = 0.0f;
  m_name = "$EMPTYANIMATION";
}