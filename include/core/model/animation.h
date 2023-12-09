#pragma once

#include "core/objects.h"

class WAnimation {
 private:
  struct StagingTransformFrame {
    float timeStamp = 0.0f;
    ETransformType transformType = ETransformType::Undefined;
    glm::vec4 transformData = glm::vec4(0.0f);
  };

  struct StagingTransformBlock {
    int32_t nodeIndex = 0;
    std::vector<StagingTransformFrame> frameData;
  };

  struct KeyFrame {
    float timeStamp = 0.0f;
    glm::mat4 matrix = glm::mat4(1.0f);
  };

  std::string m_name = "$EMPTYANIMATION$";

  // frames per second
  int32_t m_framerate = 15;

  int32_t m_totalFrameSamples = 0;

  // animation duration in seconds
  float m_duration = 0.0f;

  // staging transform data for resampling into the final keyframe data
  std::vector<StagingTransformBlock> m_stagingTransformData;

  std::unordered_map<int32_t, std::vector<KeyFrame>> m_keyframes;

 public:
  WAnimation(const std::string& inName, const int32_t inFramerate = 15)
      : m_name(inName), m_framerate(inFramerate){};

  const std::string& getName();
  void setDuration(const float duration);
  const float getDuration() const;

  std::vector<StagingTransformBlock>& getStagingTransformData();

  void addStagingEntry(const int32_t nodeIndex, const float timePoint,
                          const glm::mat4& matrix);
  void resampleStagingData(const float framerate);
  void clear();
};