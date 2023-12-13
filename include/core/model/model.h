#pragma once

#include "core/objects.h"
#include "primitive.h"

namespace tinygltf {
class Model;
class Node;
}

namespace core {
class MAnimations;
class MRenderer;
class MWorld;
}

class AEntity;
class WAnimation;

class WModel {
  friend class core::MAnimations;
  friend class core::MRenderer;
  friend class core::MWorld;
  friend class AEntity;
  struct Node;

 private:
  struct Skin {
    std::string name;
    Node* skeletonRoot = nullptr;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<Node*> joints;
    RBuffer animationBuffer;
  };

  struct Mesh {
    std::vector<std::unique_ptr<WPrimitive>> pPrimitives;
    std::vector<std::unique_ptr<WPrimitive>> pBoundingBoxes;

    // stores mesh and joints transformation matrices
    RMeshUBO uniformBlock;

    struct {
      RBuffer uniformBuffer;
      VkDescriptorBufferInfo descriptorBufferInfo{};
      VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    } uniformBufferData;

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
    int32_t index = 0;
    int32_t skinIndex = -1;

    // node hierarchy
    Node* pParentNode = nullptr;
    std::vector<std::unique_ptr<Node>> pChildren;

    // node contents
    std::unique_ptr<Mesh> pMesh;
    Skin* pSkin = nullptr;

    // node transformations (used only when resampling glTF animations)
    glm::mat4 nodeMatrix = glm::mat4(1.0f);
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(glm::vec3(0.0f));
    glm::vec3 scale = glm::vec3(1.0f);
    
    glm::mat4 transformedNodeMatrix = nodeMatrix;

    // transform matrix only for this node
    glm::mat4 getLocalMatrix();

    Node(WModel::Node* pParentNode, uint32_t index, const std::string& name);

    // allocate uniform buffer for writing transformation data
    TResult allocateMeshBuffer();
    void destroyMeshBuffer();

    void setNodeDescriptorSet(bool updateChildren = false);

    // propagate transformation through all nodes and their children
    void propagateTransformation(
        const glm::mat4& accumulatedMatrix = glm::mat4(1.0f));

    void updateStagingNodeMatrices(const glm::mat4& modelMatrix,
                                   WAnimation* pOutAnimation);
  };

  struct {
    const tinygltf::Model* pInModel = nullptr;
    uint32_t currentVertexOffset = 0u;
    uint32_t currentIndexOffset = 0u;
    std::vector<RVertex> vertices;
    std::vector<uint32_t> indices;
    RBuffer vertexBuffer;
    RBuffer indexBuffer;
    bool isClean = true;
  } staging;

  std::string m_name = "$NONAMEMODEL$";

  uint32_t m_vertexCount = 0u;
  uint32_t m_indexCount = 0u;

  std::vector<std::unique_ptr<WModel::Node>> m_pChildNodes;

  std::vector<WPrimitive*> m_pLinearPrimitives;
  std::vector<WModel::Node*> m_pLinearNodes;

  // stores indices for primitive binds in rendering manager
  // TODO: potentially deprecated, but may yet be used for depth sorting
  std::vector<uint32_t> m_primitiveBindsIndex;

  // texture samplers used by this model
  std::vector<RSamplerInfo> m_textureSamplers;

  // used materials, for glTF they have the same index as a model
  std::vector<std::string> m_materialList;

  // stored references to used animations
  std::vector<std::string> m_boundAnimations;

  // currently active animations
  std::unordered_map<std::string, int32_t> m_playingAnimations;

  // stored skins
  std::vector<std::unique_ptr<Skin>> m_pSkins;

 private:
  // common

  TResult createStagingBuffers();
  TResult validateStagingData();
  void clearStagingData();

  // sorts primitives
  void sortPrimitivesByMaterial();

  // sets root matrix for all nodes with mesh
  void update(const glm::mat4& modelMatrix) noexcept;

  // resets all transformation matrices stored in uniform blocks to identity
  void resetUniformBlockData();

  // glTF

  TResult createModel(const char* name, const tinygltf::Model* pInModel,
                      const WModelConfigInfo* pConfigInfo = nullptr);

  void prepareStagingData();

  void parseNodeProperties(const tinygltf::Node& gltfNode);

  void createNode(WModel::Node* pParentNode, const tinygltf::Node& gltfNode,
                  uint32_t gltfNodeIndex);

  // will destroy this node and its children incl. mesh and primitive contents
  void destroyNode(std::unique_ptr<WModel::Node>& pNode);

  void setTextureSamplers();

  // returned vector index corresponds to primitive material index of glTF
  void parseMaterials(const std::vector<std::string>& texturePaths);

  void extractAnimations(const WModelConfigInfo* pConfigInfo);

  void loadSkins();

  // Node

  // create simple node with a single empty mesh
  WModel::Node* createNode(WModel::Node* pParentNode, uint32_t nodeIndex,
                           std::string nodeName);

 public:
  const char* getName();
  uint32_t getVertexCount() { return m_vertexCount; }
  uint32_t getIndexCount() { return m_indexCount; }
  const std::vector<WPrimitive*>& getPrimitives();
  std::vector<uint32_t>& getPrimitiveBindsIndex();
  const std::vector<std::unique_ptr<WModel::Node>>& getRootNodes() noexcept;
  std::vector<WModel::Node*>& getAllNodes() noexcept;
  WModel::Node* getNode(uint32_t index) noexcept;

  // check if model can have the animation assigned and bind it
  void bindAnimation(const std::string& name);

  // simplified version of playAnimation
  void playAnimation(const std::string& name, const float speed = 1.0f,
                     const bool loop = true, const bool isReversed = false);

  void playAnimation(const WAnimationInfo* pAnimationInfo);

  // cleans all primitives and nodes within,
  // model itself won't get destroyed on its own
  TResult clean();
};