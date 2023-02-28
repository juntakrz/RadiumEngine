#pragma once

#include "core/objects.h"
#include "core/world/model/primitive.h"

namespace core {
class MWorld;
}

namespace tinygltf {
class Model;
class Node;
}

class WModel {
  friend class core::MWorld;

  struct Mesh {
    glm::mat4 meshMatrix = glm::mat4(1.0f);
    std::vector<std::unique_ptr<WPrimitive>> pPrimitives;
    std::vector<std::unique_ptr<WPrimitive>> pBoundingBoxes;

    struct {
      glm::vec3 min = glm::vec3(0.0f);
      glm::vec3 max = glm::vec3(0.0f);
      bool isValid = false;
    } extent;

    Mesh(){};

    bool validateBoundingBoxExtent();
  };

  struct Node {
    std::string name = "$NONAMENODE$";
    uint32_t index = 0u;
    int32_t skinIndex = -1;

    Node* pParentNode = nullptr;
    std::vector<std::unique_ptr<Node>> pChildren;

    std::unique_ptr<Mesh> pMesh;

    glm::mat4 nodeMatrix = glm::mat4(1.0f);
    glm::mat4 jointMatrix = glm::mat4(1.0f);
    float jointCount = 0.0f;

    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(glm::vec3(0.0f));
    glm::vec3 scale = glm::vec3(1.0f);

    struct {
      RBuffer uniformBuffer;
      static const uint32_t bufferSize = sizeof(glm::mat4) * 2u + sizeof(float);
      VkDescriptorBufferInfo descriptorBufferInfo{};
      VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    } uniformBufferData;

    // transform matrix only for this node
    glm::mat4 getLocalMatrix();

    // transform matrix for this and parent nodes
    glm::mat4 getMatrix();

    Node(WModel::Node* pParentNode, uint32_t index, const std::string& name);

    // update transform matrices of this node and its children
    void update();
  };

  std::string m_name = "$NONAMEMODEL$";
  uint32_t m_vertexCount = 0u;
  uint32_t m_indexCount = 0u;
  std::vector<std::unique_ptr<Node>> m_pChildNodes;

  std::vector<WPrimitive*> m_pLinearPrimitives;
  std::vector<WModel::Node*> m_pLinearNodes;

 private:
  void parseNodeProperties(const tinygltf::Model& gltfModel,
                           const tinygltf::Node& gltfNode);
  void createNode(WModel::Node* pParentNode, const tinygltf::Model& gltfModel,
                  const tinygltf::Node& gltfNode, uint32_t gltfNodeIndex);

  // create simple node with a single empty mesh
  WModel::Node* createNode(WModel::Node* pParentNode, uint32_t nodeIndex,
                           std::string nodeName);

  // will destroy this node and its children incl. mesh and primitive contents
  void destroyNode(std::unique_ptr<WModel::Node>& pNode);

  // model creation / generation methods are accessible from the World manager

 public:
  const uint32_t& getVertexCount() { return m_vertexCount; }
  const uint32_t& getIndexCount() { return m_indexCount; }
  const std::vector<WPrimitive*>& getPrimitives();

  // cleans all primitives and nodes within,
  // model itself won't get destroyed on its own
  TResult clean();
};