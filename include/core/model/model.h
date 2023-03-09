#pragma once

#include "core/objects.h"
#include "primitive.h"

namespace tinygltf {
class Model;
class Node;
}

class WModel {
  friend class core::MRenderer;
  friend class core::MWorld;
  struct Node;

  // animation data structures
  struct AnimationChannel {
    enum EPathType { TRANSLATION, ROTATION, SCALE };
    EPathType path;
    Node* node;
    uint32_t samplerIndex;
  };

  struct AnimationSampler {
    enum EInterpolationType { LINEAR, STEP, CUBICSPLINE };
    EInterpolationType interpolation;
    std::vector<float> inputs;
    std::vector<glm::vec4> outputsVec4;
  };

  struct Animation {
    std::string name;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
    float start = std::numeric_limits<float>::max();
    float end = std::numeric_limits<float>::min();
  };

  // model data structures
  struct Skin {
    std::string name;
    Node* skeletonRoot = nullptr;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<Node*> joints;
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
    uint32_t index = 0u;
    int32_t skinIndex = -1;
    
    // node hierarchy
    Node* pParentNode = nullptr;
    std::vector<std::unique_ptr<Node>> pChildren;

    // node contents
    std::unique_ptr<Mesh> pMesh;
    Skin* pSkin = nullptr;
    
    // node transformations
    glm::mat4 nodeMatrix = glm::mat4(1.0f);
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(glm::vec3(0.0f));
    glm::vec3 scale = glm::vec3(1.0f);

    // transform matrix only for this node
    glm::mat4 getLocalMatrix();

    // transform matrix for this and parent nodes
    glm::mat4 getMatrix();

    Node(WModel::Node* pParentNode, uint32_t index, const std::string& name);

    // allocate uniform buffer for writing transformation data
    TResult allocateMeshBuffer();
    void destroyMeshBuffer();

    void setNodeDescriptorSet(bool updateChildren = false);

    // update transform matrices of this node and its children
    void updateNode(const glm::mat4& modelMatrix);
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

  std::vector<std::unique_ptr<Node>> m_pChildNodes;

  std::vector<WPrimitive*> m_pLinearPrimitives;
  std::vector<WModel::Node*> m_pLinearNodes;

  // stores indices for primitive binds in rendering manager
  // TODO: potentially deprecated, but may yet be used for depth sorting
  std::vector<uint32_t> m_primitiveBindsIndex;

  // texture samplers used by this model
  std::vector<RSamplerInfo> m_textureSamplers;

  // used materials, for glTF they have the same index as a model
  std::vector<std::string> m_materialList;

  // stored animations (perhaps animations manager is needed?)
  std::vector<Animation> m_animations;

  // stored skins
  std::vector<std::unique_ptr<Skin>> m_pSkins;

 private:
   // common

  TResult createStagingBuffers();
  TResult validateStagingData();
  void clearStagingData();

  // sorts primitives
  void sortPrimitivesByMaterial();

  // glTF

  TResult createModel(const char* name, const tinygltf::Model* pInModel);

  void prepareStagingData();

  void parseNodeProperties(const tinygltf::Node& gltfNode);

  void createNode(WModel::Node* pParentNode, const tinygltf::Node& gltfNode,
                  uint32_t gltfNodeIndex);

  // will destroy this node and its children incl. mesh and primitive contents
  void destroyNode(std::unique_ptr<WModel::Node>& pNode);

  void setTextureSamplers();

  // returned vector index corresponds to primitive material index of glTF
  void parseMaterials(const std::vector<std::string>& texturePaths);

  void loadAnimations();

  void loadSkins();

  // Node

  // create simple node with a single empty mesh
  WModel::Node* createNode(WModel::Node* pParentNode, uint32_t nodeIndex,
                           std::string nodeName);

  WModel::Node* getNode(uint32_t index) noexcept;

 public:
  const char* getName();
  uint32_t getVertexCount() { return m_vertexCount; }
  uint32_t getIndexCount() { return m_indexCount; }
  const std::vector<WPrimitive*>& getPrimitives();
  std::vector<uint32_t>& getPrimitiveBindsIndex();
  const std::vector<std::unique_ptr<Node>>& getRootNodes() noexcept;
  std::vector<WModel::Node*>& getAllNodes() noexcept;

  void update(const glm::mat4& modelMatrix) noexcept;

  // cleans all primitives and nodes within,
  // model itself won't get destroyed on its own
  TResult clean();
};