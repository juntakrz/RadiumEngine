#include "pch.h"
#include "util/util.h"
#include "core/model/animation.h"

const std::string& WAnimation::getName() { return m_name; }

void WAnimation::setDuration(const float duration) { m_duration = duration; }

const float WAnimation::getDuration() const { return m_duration; }

std::vector<WAnimation::StagingTransformBlock>&
WAnimation::getStagingTransformData() {
  return m_stagingTransformData;
}

void WAnimation::addStagingEntry(const int32_t nodeIndex, const float timePoint,
                                 const glm::mat4& matrix) {
  m_keyframes[nodeIndex].emplace_back(timePoint, matrix);
}

void WAnimation::resampleStagingData(const float framerate) {
  //
}

void WAnimation::clear() {
  m_keyframes.clear();
  m_duration = 0.0f;
  m_name = "$EMPTYANIMATION";
}