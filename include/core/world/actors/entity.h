#pragma once

#include "core/world/actors/base.h"

class WModel;

namespace core {
class MAnimations;
}

class AEntity : public ABase {
  friend class core::MAnimations;

 protected:
  struct AnimatedNodeBinding {
    int32_t nodeIndex = -1;
    size_t nodeTransformBufferIndex = -1;
    size_t nodeTransformBufferOffset = -1;
  };

  EActorType m_typeId = EActorType::Entity;

  WModel* m_pModel = nullptr;
  // renderer bind index
  int32_t m_bindIndex = -1;
  // root transform buffer bind index and offset
  size_t m_rootTransformBufferIndex = -1;
  size_t m_rootTransformBufferOffset = -1;

  std::vector<AnimatedNodeBinding> m_animatedNodes;
  

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
  uint32_t getRootTransformBufferOffset();
  uint32_t getNodeTransformBufferOffset(int32_t nodeIndex);
};