#include "pch.h"
#include "util/util.h"
#include "util/math.h"
#include "core/core.h"
#include "core/model/model.h"
#include "core/world/actors/base.h"
#include "core/managers/renderer.h"
#include "core/managers/time.h"
#include "core/managers/animations.h"

core::MAnimations::MAnimations() {
  m_rootTransformBufferIndices.resize(config::scene::entityBudget);
  m_nodeTransformBufferIndices.resize(config::scene::nodeBudget);
  m_skinTransformBufferIndices.resize(config::scene::entityBudget);
}

WAnimation* core::MAnimations::createAnimation(const std::string& name) {
  if (m_animations.find(name) == m_animations.end()) {
    m_animations[name] = std::make_unique<WAnimation>(name);
    return m_animations.at(name).get();
  }

  RE_LOG(Warning,
          "Animation '%s' already exists in animations manager. A duplicate "
          "will not be created.",
          name.c_str());
  return nullptr;
}

void core::MAnimations::removeAnimation(const std::string& name) {
  if (m_animations.find(name) == m_animations.end()) {
    RE_LOG(Warning, "Failed to remove animation '%s'. It does not exist.",
           name.c_str());
    return;
  }

  // m_animations.at(inName).release();
  m_animations.erase(name);
}

WAnimation* core::MAnimations::getAnimation(const std::string& name) {
  if (m_animations.find(name) == m_animations.end()) {
    return nullptr;
  }

  return m_animations.at(name).get();
}

void core::MAnimations::clearAllAnimations() { m_animations.clear(); }

int32_t core::MAnimations::addAnimationToQueue(const WAnimationInfo* pAnimationInfo) {
  if (!pAnimationInfo) {
    RE_LOG(Error, "Failed to add animation to queue, no data was provided.");
    return -1;
  }

  // if a similar entry already exists - simply update it instead of creating a
  // new one
  QueueEntry* pExistingEntry = nullptr;

  for (QueueEntry& queuedAnimation : m_animationQueue) {
    if (queuedAnimation.pEntity == pAnimationInfo->pEntity &&
        queuedAnimation.pAnimation->getName() == pAnimationInfo->animationName) {

      pExistingEntry = &queuedAnimation;
    }
  }

  WAnimation* pAnimation = m_animations.at(pAnimationInfo->animationName).get();
  const float duration = pAnimation->m_duration;

  QueueEntry entry;
  entry.pEntity = pAnimationInfo->pEntity;
  entry.pAnimation = pAnimation;
  entry.startTime = (pAnimationInfo->startTime > duration)
                        ? duration
                        : pAnimationInfo->startTime;
  entry.endTime =
      (pAnimationInfo->endTime > duration) ? duration : pAnimationInfo->endTime;
  entry.speed = pAnimationInfo->speed;
  entry.loop = pAnimationInfo->loop;
  entry.isReversed = entry.startTime > entry.endTime;
  entry.time = entry.startTime;

  if (entry.isReversed) {
    const float endTime = entry.endTime;
    entry.endTime = entry.startTime;
    entry.startTime = endTime;
  }

  entry.duration = entry.endTime - entry.startTime;
  entry.bounce = pAnimationInfo->bounce;
  entry.queueIndex =
      (pExistingEntry) ? pExistingEntry->queueIndex : m_availableQueueIndex;

  if (!pExistingEntry) {
    m_animationQueue.emplace_back(entry);
    ++m_availableQueueIndex;
  } else {
    entry.time = pExistingEntry->time;
    memcpy(pExistingEntry, &entry, sizeof(entry));
  }

  return entry.queueIndex;
}

void core::MAnimations::runAnimationQueue() {
  cleanupQueue();
  
  for (auto& queueEntry : m_animationQueue) {
    // get a list of all nodes affected by the animation
    const auto& animatedNodes = queueEntry.pAnimation->getAnimatedNodes();
    const auto& keyFrames = queueEntry.pAnimation->getKeyFrames();

    for (const auto& node : animatedNodes) {
      AEntity::AnimatedNodeBinding* pNodeBinding = queueEntry.pEntity->getAnimatedNodeBinding(node.index);

      if (keyFrames.empty() || !pNodeBinding) {
        m_cleanupQueue.emplace_back(queueEntry.queueIndex);
        break;
      }

      const int32_t skinIndex = pNodeBinding->skinIndex;
      const bool hasSkin = !keyFrames.at(0).skinMatrices.empty();

      // write interpolated frame data directly to node's mesh uniform block
      for (size_t i = 0; i < keyFrames.size() - 1; ++i) {
        if ((queueEntry.time >= keyFrames.at(i).timeStamp) &&
            (queueEntry.time <= keyFrames.at(i + 1).timeStamp)) {
          // get interpolation coefficient based on time between frames
          float u =
              std::max(0.0f, queueEntry.time - keyFrames.at(i).timeStamp) /
              (keyFrames.at(i + 1).timeStamp - keyFrames.at(i).timeStamp);

          if (u <= 1.0f) {
            math::interpolate(keyFrames.at(i).nodeMatrices.at(node.index),
                              keyFrames.at(i + 1).nodeMatrices.at(node.index),
                              u, pNodeBinding->transformBufferBlock.nodeMatrix);

            if (hasSkin) {
              const size_t jointCount =
                  keyFrames.at(i).skinMatrices.at(skinIndex).size();

              for (int32_t j = 0; j < jointCount; ++j) {
                math::interpolate(
                  keyFrames.at(i).skinMatrices[skinIndex][j],
                  keyFrames.at(i + 1).skinMatrices[skinIndex][j], u,
                  pNodeBinding->transformBufferBlock.jointMatrices[j]);
              }
            }

            // frame update finished, exit loop
            break;
          }
        }
      }
    }

    const float timeStep = core::time.getDeltaTime() * queueEntry.speed;
    queueEntry.time += (queueEntry.isReversed) ? -timeStep : timeStep;

    if ((!queueEntry.isReversed && queueEntry.time > queueEntry.endTime) ||
        (queueEntry.isReversed && queueEntry.time < queueEntry.startTime)) {
      switch (queueEntry.loop) {
        case true: {
          switch (queueEntry.bounce) {
            case true: {
              queueEntry.isReversed = !queueEntry.isReversed;
              break;
            }
            case false: {
              queueEntry.time -= (queueEntry.isReversed) ? -queueEntry.duration
                                                         : queueEntry.duration;
              break;
            }
          }

          break;
        }
        case false: {
          m_cleanupQueue.emplace_back(queueEntry.queueIndex);
          break;
        }
      }
    }
  }
}

void core::MAnimations::cleanupQueue() {
  for (int32_t entry : m_cleanupQueue) {
    m_animationQueue.erase(m_animationQueue.begin() + entry);
  }

  m_cleanupQueue.clear();
}

bool core::MAnimations::getOrRegisterActorOffsetIndex(AEntity* pActor,
                                                      uint32_t& outIndex) {
  uint32_t freeIndex = -1;

  for (uint32_t i = 0; i < m_rootTransformBufferIndices.size(); ++i) {
    if (m_rootTransformBufferIndices[i] == nullptr && freeIndex == -1) {
      freeIndex = i;
    }

    if (m_rootTransformBufferIndices[i] == pActor) {
      outIndex = i;
      return false; // actor is already registered
    }
  }

  m_rootTransformBufferIndices[freeIndex] = pActor;
  outIndex = freeIndex;
  return true;  // registered actor to the first free index
}

bool core::MAnimations::getOrRegisterNodeOffsetIndex(
    AEntity::AnimatedNodeBinding* pNode, uint32_t& outIndex) {
  uint32_t freeIndex = -1;

  for (uint32_t i = 0; i < m_rootTransformBufferIndices.size(); ++i) {
    if (m_nodeTransformBufferIndices[i] == nullptr && freeIndex == -1) {
      freeIndex = i;
    }

    if (m_nodeTransformBufferIndices[i] == pNode) {
      outIndex = i;
      return false;  // node is already registered
    }
  }

  m_nodeTransformBufferIndices[freeIndex] = pNode;
  outIndex = freeIndex;
  return true;  // registered node to the first free index
}

bool core::MAnimations::getOrRegisterSkinOffsetIndex(AEntity::AnimatedNodeBinding* pNode, uint32_t& outIndex) {
  uint32_t freeIndex = -1;

  for (uint32_t i = 0; i < m_skinTransformBufferIndices.size(); ++i) {
    if (m_skinTransformBufferIndices[i] == nullptr && freeIndex == -1) {
      freeIndex = i;
    }

    if (m_skinTransformBufferIndices[i] == &pNode->skinIndex) {
      outIndex = i;
      return false;  // skin is already registered
    }
  }

  m_skinTransformBufferIndices[freeIndex] = &pNode->skinIndex;
  outIndex = freeIndex;

  return true;  // registered skin to the first free index
}
