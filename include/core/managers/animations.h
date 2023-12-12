#pragma once

#include "core/model/animation.h"

namespace core {
class MAnimations {
 private:
  struct FileANMNodeDescriptor {
    int32_t chunkBytes = 0;
    int32_t nodeIndex = 0;
    int32_t keyFrameCount = 0;
    int32_t jointCount = 0;
    int32_t nodeNameBytes = 0;
    std::string nodeName = "";
    std::vector<WAnimation::KeyFrame> keyFrameData;
  };

  struct FileANM {
    const int32_t magicNumber = RE_MAGIC_ANIMATIONS;
    int32_t headerBytes = 0;
    int32_t dataBytes = 0;
    int32_t animatedNodeCount = 0;
    float framerate = 0.0f;
    float duration = 0.0f;
    int32_t animationNameBytes = 0;
    std::string animationName = "";
    std::vector<FileANMNodeDescriptor> nodeData;
  };

  struct QueueEntry {
    WModel* pModel;
    WAnimation* pAnimation;
    float time = 0.0f;
    float speed = 1.0f;
    bool loop = true;
    int32_t queueIndex = 0;
    bool remove = false;
  };

  // stores all loaded animations
  std::unordered_map<std::string, std::unique_ptr<WAnimation>> m_animations;

  std::vector<QueueEntry> m_animationQueue;

  // stores queue entries that should be removed next frame
  std::vector<int32_t> m_cleanupQueue;

  int32_t m_availableQueueIndex = 0;

  MAnimations(){};

 public:
  static MAnimations& get() {
    static MAnimations _sInstance;
    return _sInstance;
  }

  MAnimations(const MAnimations&) = delete;
  MAnimations& operator=(const MAnimations&) = delete;

  WAnimation* createAnimation(const std::string& name);
  void removeAnimation(const std::string& name);
  WAnimation* getAnimation(const std::string& name);
  void clearAllAnimations();
  // load animation from file .anm, optionally rename it before adding
  TResult loadAnimation(std::string filename,
                        const std::string optionalNewName = "",
                        const std::string skeleton = "default");
  TResult saveAnimation(const std::string animation, std::string filename,
                        const std::string skeleton = "default");

  // must not be used standalone, use WModel::playAnimation() instead
  int32_t addAnimationToQueue(WAnimationPayload payload);
  void runAnimationQueue();
  void cleanupQueue();
};
}  // namespace core