#pragma once

#include "core/world/actors/base.h"

class WModel;

namespace core {
class MAnimations;
}

class AEntity : public ABase {
  friend class core::MAnimations;

 protected:
   struct AnimatedSkinBinding {
     int32_t skinIndex = -1;
     uint32_t skinTransformBufferIndex = -1;
     uint32_t skinTransformBufferOffset = -1;

     RSkinUBO transformBufferBlock;
   };

  struct AnimatedNodeBinding {
    int32_t nodeIndex = -1;
    uint32_t nodeTransformBufferIndex = -1;
    uint32_t nodeTransformBufferOffset = -1;

    int32_t skinIndex = -1;
    RNodeUBO transformBufferBlock;

    AnimatedSkinBinding* pSkinBinding = nullptr;

    bool requiresTransformBufferBlockUpdate = true;
  };

  glm::mat4 m_modelMatrix = glm::mat4(1.0f);

  EActorType m_typeId = EActorType::Entity;

  WModel* m_pModel = nullptr;
  // renderer bind index
  int32_t m_bindIndex = -1;
  // root transform buffer bind index and offset
  uint32_t m_rootTransformBufferIndex = -1;
  uint32_t m_rootTransformBufferOffset = -1;

  std::vector<AnimatedSkinBinding> m_animatedSkins;
  std::vector<AnimatedNodeBinding> m_animatedNodes;

  // currently active animations (name / index in update queue)
  std::unordered_map<std::string, int32_t> m_playingAnimations;

  AnimatedSkinBinding* getAnimatedSkinBinding(const int32_t skinIndex);
  AnimatedNodeBinding* getAnimatedNodeBinding(const int32_t nodeIndex);
  void updateTransformBuffers() noexcept;

 public:
  AEntity() noexcept {};
  virtual ~AEntity() override{};

  virtual void setModel(WModel* pModel);
  virtual WModel* getModel();
  virtual void updateModel();
  virtual void bindToRenderer();
  virtual void unbindFromRenderer();
  int32_t getRendererBindingIndex();
  void setRendererBindingIndex(int32_t bindingIndex);
  uint32_t getRootTransformBufferIndex();
  uint32_t getRootTransformBufferOffset();
  uint32_t getNodeTransformBufferIndex(int32_t nodeIndex);
  uint32_t getNodeTransformBufferOffset(int32_t nodeIndex);
  uint32_t getSkinTransformBufferIndex(int32_t skinIndex);
  uint32_t getSkinTransformBufferOffset(int32_t skinIndex);

  // simplified version of playAnimation
  void playAnimation(const std::string& name, const float speed = 1.0f,
    const bool loop = true, const bool isReversed = false);

  void playAnimation(const WAnimationInfo* pAnimationInfo);
};