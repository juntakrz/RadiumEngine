#pragma once

#include "core/world/model/mesh.h"

class WModel {

  struct Mesh {
    std::vector<WMesh*> pMeshes;
    std::vector<WMesh*> pBoundingBoxes;
  };

  struct Node {
    std::string name = "$NONAMENODE$";

    Node* pParentNode = nullptr;
    std::vector<std::unique_ptr<Node>> pChildren;

    std::unique_ptr<Mesh> pMeshData;

    glm::mat4 matrix = glm::mat4(1.0f);
    glm::mat4 jointMatrix = glm::mat4(1.0f);
    float jointCount = 0.0f;

    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(glm::vec3(0.0f));
    glm::vec3 scale = glm::vec3(1.0f);

    struct {
      RBuffer uniformBuffer;
      static const uint32_t bufferSize =
          sizeof(glm::mat4) * 2u + sizeof(float);
      VkDescriptorBufferInfo descriptorBufferInfo{};
      VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    } uniformBufferData;

    // transform matrix only for this node
    glm::mat4 getLocalMatrix();

    // transform matrix for this and parent nodes
    glm::mat4 getMatrix();

    Node();

    // update transform matrices of this node and its children
    void update();
  };

  std::string name = "$NONAMEMODEL$";

  std::vector<std::unique_ptr<Node>> pNodes;
  std::vector<std::unique_ptr<WMesh>> pMeshes;
};