#pragma once

class WAnimation {
 private:
  std::string m_name = "$EMPTYANIMATION$";

  // frames per second
  int32_t m_framerate = 15;

  int32_t m_totalFrameSamples = 0;

  // length in seconds
  float m_length = 0.0f;

  // map of joint names to which a related transformation matrix will be applied
  // every position in this vector represents a step of a 'framerate'
  std::unordered_map<std::string, std::vector<glm::mat4>> m_data;

 public:
  WAnimation(const std::string& inName, const int32_t inFramerate = 15)
      : m_name(inName), m_framerate(inFramerate){};

  const std::string& getName();
  void createEntry(const std::string& inJointName);
  void removeEntry(const std::string& inJointName);
  void addTransformationMatrix(const std::string& inJointName,
                               const glm::mat4& inMatrix);
  const glm::mat4& getTransformationMatrix(const std::string& inJointName,
                                           const int32_t inSampleNumber);
  // not error tested variant
  const glm::mat4& getTransformationMatrixFast(const std::string& inJointName,
                                               const int32_t inSampleNumber);
  void setupAnimation(const std::string& inName, const float inLengthInSeconds,
                      const int32_t inFramerate = 15);
  void clear();
};