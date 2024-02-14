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
    int32_t index;
    Node* skeletonRoot = nullptr;
    std::vector<Node*> joints;
    size_t bufferIndex = -1;   // index into skin buffer
    size_t bufferOffset = -1;  // offset in bytes into skin buffer

    RSkinUBO stagingTransformBlock;   // Used only for preprocessing animations

    struct {
      std::vector<glm::mat4> inverseBindMatrices;
      bool recalculateSkinMatrices = true;
    } staging;
  };

  struct Mesh {
    int32_t index = -1;
    std::vector<std::unique_ptr<WPrimitive>> pPrimitives;
    std::vector<std::unique_ptr<WPrimitive>> pBoundingBoxes;

    RNodeUBO stagingTransformBlock;   // Used only for preprocessing animations
                                      // Storing node transformation only for nodes with mesh data
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
    int32_t index = -1;
    int32_t skinIndex = -1;
    bool isRequestingTransformBufferUpdate = true;

    // node hierarchy
    Node* pParentNode = nullptr;
    std::vector<std::unique_ptr<Node>> pChildren;

    // node contents
    std::unique_ptr<Mesh> pMesh;
    Skin* pSkin = nullptr;

    // node transformations (used only when resampling glTF animations)
    struct {
      glm::mat4 nodeMatrix = glm::mat4(1.0f);
      glm::vec3 translation = glm::vec3(0.0f);
      glm::quat rotation = glm::quat(glm::vec3(0.0f));
      glm::vec3 scale = glm::vec3(1.0f);
    } staging;
    
    glm::mat4 transformedNodeMatrix = staging.nodeMatrix;

    // transform matrix only for this node
    glm::mat4 getLocalMatrix();

    Node(WModel::Node* pParentNode, uint32_t index, const std::string& name);

    // propagate transformation through all nodes and their children
    void propagateTransformation(
        const glm::mat4& accumulatedMatrix = glm::mat4(1.0f));

    void updateStagingNodeMatrices(WAnimation* pOutAnimation);
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

  uint32_t m_sceneVertexOffset = 0u;
  uint32_t m_sceneIndexOffset = 0u;
  uint32_t m_vertexCount = 0u;
  uint32_t m_indexCount = 0u;
  int32_t m_meshCount = 0;

  std::vector<std::unique_ptr<WModel::Node>> m_pChildNodes;

  std::vector<WPrimitive*> m_pLinearPrimitives;
  std::vector<WModel::Node*> m_pLinearNodes;
  std::vector<WModel::Mesh*> m_pLinearMeshes;

  // stores indices for primitive binds in rendering manager
  // TODO: potentially deprecated, but may yet be used for depth sorting
  std::vector<uint32_t> m_primitiveBindsIndex;

  // texture samplers used by this model
  std::vector<RSamplerInfo> m_textureSamplers;

  // used materials, for glTF they have the same index as a model
  std::vector<std::string> m_materialList;

  // stored references to used animations
  std::vector<std::string> m_boundAnimations;

  // stored skins
  std::vector<std::unique_ptr<Skin>> m_pSkins;

 private:
  // common

  TResult createStagingBuffers();
  TResult validateStagingData();
  void clearStagingData();

  // sorts primitives
  void sortPrimitivesByMaterial();

  // update node transform buffer at node offsets
  void updateNodeTransformBuffer(int32_t nodeIndex,
                                 uint32_t bufferOffset) noexcept;

  void updateSkinTransformBuffer() noexcept;

  // resets all transformation matrices stored in uniform blocks to identity
  void resetUniformBlockData();

  void uploadToSceneBuffer();

 public:
  WPrimitive* getPrimitive(const int32_t meshIndex,
                           const int32_t primitiveIndex);
  void setPrimitiveMaterial(const int32_t meshIndex,
                            const int32_t primitiveIndex, const char* material);

  // glTF
 private:

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
  WModel::Node* getNode(int32_t index) noexcept;
  WModel::Node* getNodeBySkinIndex(int32_t index) noexcept;
  int32_t getSkinCount() noexcept;
  WModel::Skin* getSkin(int32_t skinIndex) noexcept;

  // check if model can have the animation assigned and bind it
  void bindAnimation(const std::string& name);

  // cleans all primitives and nodes within,
  // model itself won't get destroyed on its own
  TResult clean();
};