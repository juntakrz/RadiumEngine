#pragma once

#include "core/world/actors/abase.h"
#include "core/world/mesh/mesh.h"

class WMesh;

class AModel : public ABase {
 protected:
  EActorType m_typeId = EActorType::Model;

  std::string name = "$NONAMEMODEL$";

  struct SModelNode {
    std::string name = "$NONAMENODE$";

    SModelNode* pParentNode = nullptr;
    std::vector<std::unique_ptr<SModelNode>> pChildren;

    std::vector<WMesh*> pMeshes;
    std::vector<WMesh*> pBoundingBoxes;

    glm::mat4 matrix;
    glm::vec3 translation;
    glm::vec3 scale;
    glm::vec3 rotation;

    // transform matrix only for this node
    glm::mat4 getLocalMatrix();

    // transform matrix for this and parent nodes
    glm::mat4 getMatrix();

    // update transform matrices of this node and its children
    void update();
  };

  std::vector<std::unique_ptr<SModelNode>> pNodes;
  std::vector<std::unique_ptr<WMesh>> pMeshes;

 public:
  AModel() noexcept {};
  virtual ~AModel() override{};
};