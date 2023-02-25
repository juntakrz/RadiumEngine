#pragma once

#include "core/world/actors/abase.h"

class WMesh;

class AModel : public ABase {
 protected:
  EActorType m_typeId = EActorType::Model;

  struct SModelNode {
    std::vector<std::unique_ptr<SModelNode>> pChildNodes;
    std::vector<WMesh*> pMeshes;
    std::vector<WMesh*> pAABBs;

    glm::mat4 nodeTransform;
  };

 public:
  AModel() noexcept {};
  virtual ~AModel() override{};
};