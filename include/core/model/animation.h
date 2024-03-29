#pragma once

#include "core/objects.h"

namespace core {
class MAnimations;
}

class WModel;

class WAnimation {
  friend class core::MAnimations;

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

  // staging transform data for resampling into the final keyframe data
  struct {
    std::vector<StagingTransformBlock> transformData;
    float startTimeStamp = 0.0f;
    float endTimeStamp = 0.0f;
    float duration = 0.0f;
  } stagingData;

  struct AnimatedNode {
    std::string name = "";
    int32_t index = 0;
  };

  struct KeyFrame {
    float timeStamp = 0.0f;
    std::unordered_map<int32_t, glm::mat4> nodeMatrices;
    std::vector<std::vector<glm::mat4>> skinMatrices;
  };

  std::string m_name = "$EMPTYANIMATION$";

  // frames per second
  float m_framerate = 15.0f;

  // animation duration in seconds
  float m_duration = 0.0f;

  // contains node index and name that this animation affects
  std::vector<AnimatedNode> m_animatedNodes;

  // contains time stamps and animated node matrices
  std::vector<KeyFrame> m_keyFrames;

  void processFrame(WModel* pModel, const float time);
  void addKeyFrame(WModel* pModel, const float timeStamp);

 public:
  WAnimation(const std::string& name) : m_name(name){};

  const std::string& getName();
  void setStagingTimeRange(const float startTime, const float endTime);
  void setDuration(const float duration);
  const float getDuration() const;

  bool addNodeReference(const std::string& name, const int32_t index);
  bool checkNodeReference(const int32_t index);
  bool removeNodeReference(const std::string& name);
  bool removeNodeReference(const int32_t index);
  void clearNodeReferences();

  std::vector<StagingTransformBlock>& getStagingTransformData();
  void clearStagingTransformData();

  void resampleKeyFrames(WModel* pModel, const float framerate,
                         const float speed = 1.0f);
  const std::vector<KeyFrame>& getKeyFrames();

  // check if the model has all the required nodes for this animation
  // the number of nodes in this animation is allowed to be lower
  // than total animated nodes of the model
  bool validateModel(WModel* pModel);

  const std::vector<WAnimation::AnimatedNode>& getAnimatedNodes();
};