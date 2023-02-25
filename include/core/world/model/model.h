#pragma once

#include "core/world/model/mesh.h"

class WModel {
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
};