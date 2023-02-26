#pragma once

#include "core/world/model/mesh.h"

class WModel {
  std::string name = "$NONAMEMODEL$";

  struct ModelNode {
    std::string name = "$NONAMENODE$";

    ModelNode* pParentNode = nullptr;
    std::vector<std::unique_ptr<ModelNode>> pChildren;

    std::vector<WMesh*> pMeshes;
    std::vector<WMesh*> pBoundingBoxes;

    glm::mat4 matrix = glm::mat4(1.0f);
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(glm::vec3(0.0f));
    glm::vec3 scale = glm::vec3(1.0f);

    // transform matrix only for this node
    glm::mat4 getLocalMatrix();

    // transform matrix for this and parent nodes
    glm::mat4 getMatrix();

    // update transform matrices of this node and its children
    void update();
  };

  std::vector<std::unique_ptr<ModelNode>> pNodes;
  std::vector<std::unique_ptr<WMesh>> pMeshes;
};