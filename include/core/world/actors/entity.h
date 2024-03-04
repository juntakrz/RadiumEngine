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

  EActorType m_typeId = EActorType::Entity;

  WModel* m_pModel = nullptr;
  
  // Renderer bind index
  int32_t m_bindIndex = -1;

  // Model instance index, must be bound to renderer to get a valid one
  uint32_t m_instanceIndex = -1;

  // Root transform buffer bind index and offset
  uint32_t m_rootTransformBufferIndex = -1;
  uint32_t m_rootTransformBufferOffset = -1;

  std::vector<AnimatedSkinBinding> m_animatedSkins;
  std::vector<AnimatedNodeBinding> m_animatedNodes;

  // Currently active animations (name / index in update queue)
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

  void setInstancePrimitiveMaterial(const int32_t meshIndex,
    const int32_t primitiveIndex, const char* material);

  // simplified version of playAnimation
  void playAnimation(const std::string& name, const float speed = 1.0f,
    const bool loop = true, const bool isReversed = false);

  void playAnimation(const WAnimationInfo* pAnimationInfo);
};