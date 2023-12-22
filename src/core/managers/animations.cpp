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
    if (queuedAnimation.pModel == pAnimationInfo->pModel &&
        queuedAnimation.pAnimation->getName() == pAnimationInfo->animationName) {

      pExistingEntry = &queuedAnimation;
    }
  }

  WAnimation* pAnimation = m_animations.at(pAnimationInfo->animationName).get();
  const float duration = pAnimation->m_duration;

  QueueEntry entry;
  entry.pModel = pAnimationInfo->pModel;
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
  /*
  for (auto& queueEntry : m_animationQueue) {
    // get a list of all nodes affected by the animation
    const auto& animatedNodes = queueEntry.pAnimation->getAnimatedNodes();

    for (const auto& node : animatedNodes) {
      const auto& nodeKeyFrames =
          queueEntry.pAnimation->getNodeKeyFrames(node.index);

      WModel::Node* pNode = queueEntry.pModel->getNode(node.index);

      if (!nodeKeyFrames || !pNode) {
        m_cleanupQueue.emplace_back(queueEntry.queueIndex);
        break;
      }

      // write interpolated frame data directly to node's mesh uniform block
      for (size_t i = 0; i < nodeKeyFrames->size() - 1; ++i) {
        // if we are at the last frame - use first frame for interpolation
        // const size_t iAux = (i == nodeKeyFrames->size() - 1) ? 0 : i + 1;

        if ((queueEntry.time >= nodeKeyFrames->at(i).timeStamp) &&
            (queueEntry.time <= nodeKeyFrames->at(i + 1).timeStamp)) {
          // get interpolation coefficient based on time between frames
          float u =
              std::max(0.0f, queueEntry.time - nodeKeyFrames->at(i).timeStamp) /
              (nodeKeyFrames->at(i + 1).timeStamp -
               nodeKeyFrames->at(i).timeStamp);

          if (u <= 1.0f) {
            math::interpolate(
                nodeKeyFrames->at(i).nodeMatrices.at(node.index),
                nodeKeyFrames->at(i + 1).nodeMatrices.at(node.index), u,
                pNode->pMesh->uniformBlock.nodeMatrix);

            const size_t jointCount = nodeKeyFrames->at(i).jointMatrices.size();

            for (int32_t j = 0; j < jointCount; ++j) {
              math::interpolate(nodeKeyFrames->at(i).jointMatrices[j],
                                nodeKeyFrames->at(i + 1).jointMatrices[j], u,
                                pNode->pMesh->uniformBlock.jointMatrices[j]);
            }

            pNode->pMesh->uniformBlock.jointCount = (float)jointCount;

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
  }*/
}

void core::MAnimations::cleanupQueue() {
  for (int32_t entry : m_cleanupQueue) {
    m_animationQueue.erase(m_animationQueue.begin() + entry);
  }

  m_cleanupQueue.clear();
}

bool core::MAnimations::getOrRegisterActorOffsetIndex(AEntity* pActor,
                                                      size_t& outIndex) {
  size_t freeIndex = -1;

  for (size_t i = 0; i < m_rootTransformBufferIndices.size(); ++i) {
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
    AEntity::AnimatedNodeBinding* pNode, size_t& outIndex) {
  size_t freeIndex = -1;

  for (size_t i = 0; i < m_rootTransformBufferIndices.size(); ++i) {
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

bool core::MAnimations::getOrRegisterSkinOffsetIndex(WModel::Skin* pSkin,
                                                     size_t& outIndex) {
  size_t freeIndex = -1;

  for (size_t i = 0; i < m_skinTransformBufferIndices.size(); ++i) {
    if (m_skinTransformBufferIndices[i] == nullptr && freeIndex == -1) {
      freeIndex = i;
    }

    if (m_skinTransformBufferIndices[i] == pSkin) {
      outIndex = i;
      return false;  // skin is already registered
    }
  }

  m_skinTransformBufferIndices[freeIndex] = pSkin;
  outIndex = freeIndex;

  return true;  // registered skin to the first free index
}
