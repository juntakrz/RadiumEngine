#include "pch.h"
#include "util/util.h"
#include "core/model/model.h"
#include "core/model/animation.h"

//
// PRIVATE
//

void WAnimation::processFrame(WModel* pModel, const float time) {
  // iterate through staged transformation blocks
  for (auto& transformBlock : stagingData.transformData) {
    auto* pNode = pModel->getNode(transformBlock.nodeIndex);

    // iterate through frames and retrieve transformation data at a desired time
    for (size_t i = 0; i < transformBlock.frameData.size() - 1; ++i) {
      if ((time >= transformBlock.frameData[i].timeStamp) &&
          (time <= transformBlock.frameData[i + 1].timeStamp)) {
        // get interpolation coefficient based on time between frames
        float u = std::max(0.0f, time - transformBlock.frameData[i].timeStamp) /
                  (transformBlock.frameData[i + 1].timeStamp -
                   transformBlock.frameData[i].timeStamp);

        if (u <= 1.0f) {
          switch (transformBlock.frameData[i].transformType) {
            case ETransformType::Translation: {
              glm::vec4 translation =
                  glm::mix(transformBlock.frameData[i].transformData,
                           transformBlock.frameData[i + 1].transformData, u);
              pNode->translation = glm::vec3(translation);
              break;
            }
            case ETransformType::Scale: {
              glm::vec4 scale =
                  glm::mix(transformBlock.frameData[i].transformData,
                           transformBlock.frameData[i + 1].transformData, u);
              pNode->scale = glm::vec3(scale);
              break;
            }
            case ETransformType::Rotation: {
              glm::quat q1;
              q1.x = transformBlock.frameData[i].transformData.x;
              q1.y = transformBlock.frameData[i].transformData.y;
              q1.z = transformBlock.frameData[i].transformData.z;
              q1.w = transformBlock.frameData[i].transformData.w;

              glm::quat q2;
              q2.x = transformBlock.frameData[i + 1].transformData.x;
              q2.y = transformBlock.frameData[i + 1].transformData.y;
              q2.z = transformBlock.frameData[i + 1].transformData.z;
              q2.w = transformBlock.frameData[i + 1].transformData.w;

              pNode->rotation = glm::normalize(glm::slerp(q1, q2, u));
              break;
            }
          }
          // transform block data retrieved, exit loop
          break;
        }
      }
    }
  }

  // create accumulated matrices for every node
  for (auto& rootNode : pModel->getRootNodes()) {
    // propagate node transformation accumulating from parent to children
    rootNode->propagateTransformation();

    // set root and propagated transformation matrices and update joint matrices
    rootNode->updateStagingNodeMatrices(glm::mat4(1.0f), this);
  }

  // create a keyframe using accumulated matrices
  for (auto& node : pModel->getAllNodes()) {
    if (node->pMesh) {
      addKeyFrame(node->index, time - stagingData.startTimeStamp,
                  node->pMesh->uniformBlock);
    }
  }
}

void WAnimation::addKeyFrame(const int32_t nodeIndex, const float timeStamp,
                             const RMeshUBO& meshUBO) {
  m_nodeKeyFrames[nodeIndex].emplace_back(timeStamp, meshUBO.nodeMatrix);

  KeyFrame& keyframe = m_nodeKeyFrames[nodeIndex].back();
  const int32_t jointCount = static_cast<int32_t>(meshUBO.jointCount);

  for (int32_t i = 0; i < jointCount; ++i) {
    keyframe.jointMatrices.emplace_back(meshUBO.jointMatrices[i]);
  }
}

void WAnimation::addKeyFrame(const int32_t nodeIndex, const float timeStamp,
                             const glm::mat4& nodeMatrix,
                             const std::vector<glm::mat4>& jointMatrices) {
  m_nodeKeyFrames[nodeIndex].emplace_back(timeStamp, nodeMatrix, jointMatrices);
}

//
// PUBLIC
//

const std::string& WAnimation::getName() { return m_name; }

void WAnimation::setStagingTimeRange(const float startTime,
                                     const float endTime) {
  stagingData.startTimeStamp = startTime;
  stagingData.endTimeStamp = endTime;

  stagingData.duration = endTime - startTime;
}

void WAnimation::setDuration(const float duration) { m_duration = duration; }

const float WAnimation::getDuration() const { return m_duration; }

bool WAnimation::addNodeReference(const std::string& name,
                                  const int32_t index) {
  for (const auto& node : m_animatedNodes) {
    if (strcmp(node.name.c_str(), name.c_str()) == 0) {
      return false;
    }

    if (node.index == index) {
      return false;
    }
  }

  m_animatedNodes.emplace_back(name, index);
  return true;
}

bool WAnimation::checkNodeReference(const int32_t index) {
  for (const auto& node : m_animatedNodes) {
    if (node.index == index) {
      return true;
    }
  }

  return false;
}

bool WAnimation::removeNodeReference(const std::string& name) {
  uint32_t index = 0;

  for (auto& node : m_animatedNodes) {
    if (node.name == name) {
      m_animatedNodes.erase(m_animatedNodes.begin() + index);
      return true;
    }
  }

  return false;
}

bool WAnimation::removeNodeReference(const int32_t index) {
  uint32_t i = 0;

  for (auto& node : m_animatedNodes) {
    if (node.index == index) {
      m_animatedNodes.erase(m_animatedNodes.begin() + i);
      return true;
    }
  }

  return false;
}

void WAnimation::clearNodeReferences() { m_animatedNodes.clear(); }

std::vector<WAnimation::StagingTransformBlock>&
WAnimation::getStagingTransformData() {
  return stagingData.transformData;
}

void WAnimation::clearStagingTransformData() {
  stagingData.duration = 0.0f;
  stagingData.startTimeStamp = 0.0f;
  stagingData.endTimeStamp = 0.0f;
  stagingData.transformData.clear();
}

void WAnimation::resampleKeyFrames(WModel* pModel, const float framerate,
                                          const float speed) {
  if (stagingData.transformData.empty() || !pModel) {
    RE_LOG(Error,
           "Can't resample frame data for animation '%s'. Required data is "
           "missing.",
           m_name);

    return;
  }

  const float timeStep = 1.0f / framerate * 1.0f;
  float time = stagingData.startTimeStamp;

  m_nodeKeyFrames.clear();

  while (time < stagingData.endTimeStamp) {
    processFrame(pModel, time);
    time += timeStep;
  }

  m_framerate = framerate;

  // get final animation duration
  float resampledDuration = 0.0f;

  for (const auto& keyFrameVector : m_nodeKeyFrames) {
    const float lastFrameTime = keyFrameVector.second.back().timeStamp;

    if (resampledDuration < lastFrameTime) {
      resampledDuration = lastFrameTime;
    }
  }

  m_duration = resampledDuration;
}

bool WAnimation::validateModel(WModel* pModel) {
  if (!pModel) {
    RE_LOG(
        Error,
        "Failed to validate model for animation '%s', no model was provided.",
        m_name.c_str());
    return false;
  }

  const auto& nodeVector = pModel->getAllNodes();
  int32_t matches = 0;

  for (const auto& animatedNode : m_animatedNodes) {
    for (const auto& node : nodeVector) {
      if (node->index == animatedNode.index &&
          strcmp(node->name.c_str(), animatedNode.name.c_str()) == 0) {
        ++matches;
      }
    }
  }

  if (matches == m_animatedNodes.size()) {
    return true;
  }

  return false;
}

const std::vector<WAnimation::AnimatedNode>& WAnimation::getAnimatedNodes() {
  return m_animatedNodes;
}

const std::vector<WAnimation::KeyFrame>* WAnimation::getNodeKeyFrames(
    const int32_t nodeIndex) {
  if (m_nodeKeyFrames.find(nodeIndex) == m_nodeKeyFrames.end()) {
    RE_LOG(Error,
           "Failed to get node keyframes for animation '%s'. Node with index '%d' "
           "does not exist.",
           m_name.c_str(), nodeIndex);

    return nullptr;
  }

  return &m_nodeKeyFrames.at(nodeIndex);
}