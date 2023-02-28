#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/world/model/model.h"

#include "tinygltf/tiny_gltf.h"

glm::mat4 WModel::Node::getLocalMatrix() {
  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
         glm::scale(glm::mat4(1.0f), scale) * nodeMatrix;
}

glm::mat4 WModel::Node::getMatrix() {
  glm::mat4 matrix = getLocalMatrix();
  Node* pParent = pParentNode;
  while (pParent) {
    matrix = pParent->getLocalMatrix() * matrix;
    pParent = pParent->pParentNode;
  }
  return matrix;
}

WModel::Node::Node(WModel::Node* pParent, uint32_t index,
                   const std::string& name)
    : pParentNode(pParent), index(index), name(name) {
  // prepare buffer and memory to store node transformation matrix
  core::renderer.createBuffer(EBCMode::CPU_UNIFORM,
                              uniformBufferData.bufferSize,
                              uniformBufferData.uniformBuffer);
}

void WModel::Node::update() {
  if (!pMeshData) {
    glm::mat4 matrix = getMatrix();
    /*if (skin) {
      mesh->uniformBlock.matrix = m;
      // Update join matrices
      glm::mat4 inverseTransform = glm::inverse(m);
      size_t numJoints =
          std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
      for (size_t i = 0; i < numJoints; i++) {
        vkglTF::Node* jointNode = skin->joints[i];
        glm::mat4 jointMat =
            jointNode->getMatrix() * skin->inverseBindMatrices[i];
        jointMat = inverseTransform * jointMat;
        mesh->uniformBlock.jointMatrix[i] = jointMat;
      }
      mesh->uniformBlock.jointcount = (float)numJoints;
      memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock,
             sizeof(mesh->uniformBlock));
    } else {
      memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
    }*/

    // store node matrix into a uniform buffer
    memcpy(uniformBufferData.uniformBuffer.allocInfo.pMappedData, &matrix,
           sizeof(glm::mat4));
  }

  for (auto& pChild : pChildren) {
    pChild->update();
  }
}

void WModel::parseNodeProperties(const tinygltf::Model& gltfModel,
  const tinygltf::Node& gltfNode) {
  if (gltfNode.children.size() > 0) {
    for (size_t i = 0; i < gltfNode.children.size(); i++) {
      parseNodeProperties(gltfModel, gltfModel.nodes[gltfNode.children[i]]);
    }
  }

  if (gltfNode.mesh > -1) {
    const tinygltf::Mesh& mesh = gltfModel.meshes[gltfNode.mesh];

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
      const tinygltf::Primitive& currentPrimitive = mesh.primitives[i];

      // get vertex count of a current primitive
      m_vertexCount += static_cast<uint32_t>(
          gltfModel.accessors[currentPrimitive.attributes.find("POSITION")->second]
              .count);

      // get index count of a current primitive
      if (currentPrimitive.indices > -1) {
        m_indexCount += static_cast<uint32_t>(
            gltfModel.accessors[currentPrimitive.indices].count);
      }
    }
  }
}

void WModel::createNode(WModel::Node* pParentNode,
                        const tinygltf::Model& gltfModel,
                        const tinygltf::Node& gltfNode, uint32_t gltfNodeIndex) {
  m_pNodes.emplace_back(std::make_unique<WModel::Node>(
      pParentNode, gltfNodeIndex, gltfNode.name));

  Node* pNode = m_pNodes.back().get();
  pNode->skinIndex = gltfNode.skin;
  pNode->nodeMatrix = glm::mat4(1.0f);

  // general local node matrix
  if (gltfNode.translation.size() == 3) {
    pNode->translation = glm::make_vec3(gltfNode.translation.data());
  }

  if (gltfNode.rotation.size() == 4) {
    glm::quat q = glm::make_quat(gltfNode.rotation.data());
    pNode->rotation = glm::mat4(q);   // why if both are quaternions? check later
  }

  if (gltfNode.scale.size() == 3) {
    pNode->scale = glm::make_vec3(gltfNode.scale.data());
  }

  if (gltfNode.matrix.size() == 14) {
    pNode->nodeMatrix = glm::make_mat4x4(gltfNode.matrix.data());
  }

  // recursively create node children
  if (gltfNode.children.size() > 0) {
    for (size_t i = 0; i < gltfNode.children.size(); ++i) {
      createNode(pNode, gltfModel, gltfModel.nodes[gltfNode.children[i]],
                 gltfNode.children[i]);
    }
  }

  // retrieve mesh data if available
  if (gltfNode.mesh > -1) {

  }
}