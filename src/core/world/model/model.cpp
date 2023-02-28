#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/world/model/primitive_custom.h"
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
  if (!pMesh) {
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
                        const tinygltf::Node& gltfNode,
                        uint32_t gltfNodeIndex) {
  WModel::Node* pNode = nullptr;

  if (pParentNode == nullptr) {
    m_pChildNodes.emplace_back(std::make_unique<WModel::Node>(
        pParentNode, gltfNodeIndex, gltfNode.name));
    pNode = m_pChildNodes.back().get();
  } else {
    pParentNode->pChildren.emplace_back(std::make_unique<WModel::Node>(
        pParentNode, gltfNodeIndex, gltfNode.name));
    pNode = pParentNode->pChildren.back().get();
  }

  if (!pNode) {
    RE_LOG(Error, "Trying to create the node '%s', but got nullptr.",
           gltfNode.name.c_str());
    return;
  }
  
  pNode->skinIndex = gltfNode.skin;
  pNode->nodeMatrix = glm::mat4(1.0f);

  // general local node matrix
  if (gltfNode.translation.size() == 3) {
    pNode->translation = glm::make_vec3(gltfNode.translation.data());
  }

  if (gltfNode.rotation.size() == 4) {
    glm::quat q = glm::make_quat(gltfNode.rotation.data());
    pNode->rotation = glm::mat4(q);  // why if both are quaternions? check later
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
    const tinygltf::Mesh& gltfMesh = gltfModel.meshes[gltfNode.mesh];
    pNode->pMesh = std::make_unique<WModel::Mesh>();
    WModel::Mesh* pMesh = pNode->pMesh.get();

    for (size_t j = 0; j < gltfMesh.primitives.size(); ++j) {
      const tinygltf::Primitive& gltfPrimitive = gltfMesh.primitives[j];
      glm::vec3 posMin{0.0f}, posMax{0.0f};
      uint32_t vertexStart = 0, indexStart = 0, vertexCount = 0, indexCount = 0;
      bool hasSkin = false, hasIndices = gltfPrimitive.indices > -1;

      // prepare an array of RVertex and indices to store vertex and index data
      std::vector<RVertex> vertices;
      std::vector<uint32_t> indices;

      // VERTICES
      {
        const float* pBufferPos = nullptr;
        const float* pBufferNormals = nullptr;
        const float* pBufferTexCoords0 = nullptr;
        const float* pBufferTexCoords1 = nullptr;
        const float* pBufferColors = nullptr;
        const void* pBufferJoints = nullptr;
        const float* pBufferWeights = nullptr;

        int32_t posByteStride = 0;
        int32_t normalsByteStride = 0;
        int32_t tex0ByteStride = 0;
        int32_t tex1ByteStride = 0;
        int32_t colorsByteStride = 0;
        int32_t jointByteStride = 0;
        int32_t weightByteStride = 0;

        int32_t jointComponentType = 0;

        if (!gltfPrimitive.attributes.contains("POSITION")) {
          RE_LOG(Error,
                 "GLTF primitive is invalid, no proper vertex data was found.");
          return;
        }

        const tinygltf::Accessor& posAccessor =
            gltfModel.accessors[gltfPrimitive.attributes.at("POSITION")];
        const tinygltf::BufferView& posBufferView =
            gltfModel.bufferViews[posAccessor.bufferView];
        pBufferPos = reinterpret_cast<const float*>(
            &(gltfModel.buffers[posBufferView.buffer]
                  .data[posAccessor.byteOffset + posBufferView.byteOffset]));
        posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1],
                           posAccessor.minValues[2]);
        posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1],
                           posAccessor.maxValues[2]);
        vertexCount = static_cast<uint32_t>(posAccessor.count);
        posByteStride =
            posAccessor.ByteStride(posBufferView)
                ? (posAccessor.ByteStride(posBufferView) / sizeof(float))
                : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

        // normals
        if (gltfPrimitive.attributes.contains("NORMAL")) {
          const tinygltf::Accessor& normAccessor =
              gltfModel.accessors[gltfPrimitive.attributes.at("NORMAL")];
          const tinygltf::BufferView& normView =
              gltfModel.bufferViews[normAccessor.bufferView];
          pBufferNormals = reinterpret_cast<const float*>(
              &(gltfModel.buffers[normView.buffer]
                    .data[normAccessor.byteOffset + normView.byteOffset]));
          normalsByteStride =
              normAccessor.ByteStride(normView)
                  ? (normAccessor.ByteStride(normView) / sizeof(float))
                  : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
        }

        // UVs
        if (gltfPrimitive.attributes.contains("TEXCOORD_0")) {
          const tinygltf::Accessor& uvAccessor =
              gltfModel.accessors[gltfPrimitive.attributes.at("TEXCOORD_0")];
          const tinygltf::BufferView& uvView =
              gltfModel.bufferViews[uvAccessor.bufferView];
          pBufferTexCoords0 = reinterpret_cast<const float*>(
              &(gltfModel.buffers[uvView.buffer]
                    .data[uvAccessor.byteOffset + uvView.byteOffset]));
          tex0ByteStride =
              uvAccessor.ByteStride(uvView)
                  ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                  : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
        }
        if (gltfPrimitive.attributes.contains("TEXCOORD_1")) {
          const tinygltf::Accessor& uvAccessor =
              gltfModel.accessors[gltfPrimitive.attributes.at("TEXCOORD_1")];
          const tinygltf::BufferView& uvView =
              gltfModel.bufferViews[uvAccessor.bufferView];
          pBufferTexCoords1 = reinterpret_cast<const float*>(
              &(gltfModel.buffers[uvView.buffer]
                    .data[uvAccessor.byteOffset + uvView.byteOffset]));
          tex1ByteStride =
              uvAccessor.ByteStride(uvView)
                  ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                  : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
        }

        // vertex colors
        if (gltfPrimitive.attributes.contains("COLOR_0")) {
          const tinygltf::Accessor& accessor =
              gltfModel.accessors[gltfPrimitive.attributes.at("COLOR_0")];
          const tinygltf::BufferView& view =
              gltfModel.bufferViews[accessor.bufferView];
          pBufferColors = reinterpret_cast<const float*>(
              &(gltfModel.buffers[view.buffer]
                    .data[accessor.byteOffset + view.byteOffset]));
          colorsByteStride =
              accessor.ByteStride(view)
                  ? (accessor.ByteStride(view) / sizeof(float))
                  : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
        }

        // skinning and joints
        if (gltfPrimitive.attributes.contains("JOINTS_0")) {
          const tinygltf::Accessor& jointAccessor =
              gltfModel.accessors[gltfPrimitive.attributes.at("JOINTS_0")];
          const tinygltf::BufferView& jointView =
              gltfModel.bufferViews[jointAccessor.bufferView];
          pBufferJoints =
              &(gltfModel.buffers[jointView.buffer]
                    .data[jointAccessor.byteOffset + jointView.byteOffset]);
          jointComponentType = jointAccessor.componentType;
          jointByteStride =
              jointAccessor.ByteStride(jointView)
                  ? (jointAccessor.ByteStride(jointView) /
                     tinygltf::GetComponentSizeInBytes(jointComponentType))
                  : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
        }

        if (gltfPrimitive.attributes.contains("WEIGHTS_0")) {
          const tinygltf::Accessor& weightAccessor =
              gltfModel.accessors[gltfPrimitive.attributes.at("WEIGHTS_0")];
          const tinygltf::BufferView& weightView =
              gltfModel.bufferViews[weightAccessor.bufferView];
          pBufferWeights = reinterpret_cast<const float*>(
              &(gltfModel.buffers[weightView.buffer]
                    .data[weightAccessor.byteOffset + weightView.byteOffset]));
          weightByteStride =
              weightAccessor.ByteStride(weightView)
                  ? (weightAccessor.ByteStride(weightView) / sizeof(float))
                  : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
        }

        hasSkin = (pBufferJoints && pBufferWeights);

        vertices.resize(posAccessor.count);

        for (size_t v = 0; v < posAccessor.count; ++v) {
          RVertex& vertex = vertices[vertexStart + v];
          vertex.pos = glm::make_vec3(&pBufferPos[v * posByteStride]);
          vertex.normal = glm::normalize(glm::vec3(
              pBufferNormals
                  ? glm::make_vec3(&pBufferNormals[v * normalsByteStride])
                  : glm::vec3(0.0f)));
          vertex.tex0 =
              pBufferTexCoords0
                  ? glm::make_vec2(&pBufferTexCoords0[v * tex0ByteStride])
                  : glm::vec2(0.0f);
          vertex.tex1 =
              pBufferTexCoords1
                  ? glm::make_vec2(&pBufferTexCoords1[v * tex1ByteStride])
                  : glm::vec2(0.0f);
          vertex.color =
              pBufferColors
                  ? glm::make_vec4(&pBufferColors[v * colorsByteStride])
                  : glm::vec4(1.0f);

          if (hasSkin) {
            switch (jointComponentType) {
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                const uint16_t* pBuffer =
                    static_cast<const uint16_t*>(pBufferJoints);
                vertex.joint =
                    glm::vec4(glm::make_vec4(&pBuffer[v * jointByteStride]));
                break;
              }
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                const uint8_t* pBuffer =
                    static_cast<const uint8_t*>(pBufferJoints);
                vertex.joint =
                    glm::vec4(glm::make_vec4(&pBuffer[v * jointByteStride]));
                break;
              }
              default:
                RE_LOG(Error, "Joint component type %d not supported",
                       jointComponentType);
                break;
            }
          } else {
            vertex.joint = glm::vec4(0.0f);
          }
          vertex.weight =
              hasSkin ? glm::make_vec4(&pBufferWeights[v * weightByteStride])
                      : glm::vec4(0.0f);
          // fix for all zero weights
          if (glm::length(vertex.weight) == 0.0f) {
            vertex.weight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
          }
        }
      }

      // INDICES
      if (hasIndices) {
        const tinygltf::Accessor& accessor =
            gltfModel
                .accessors[gltfPrimitive.indices > -1 ? gltfPrimitive.indices
                                                      : 0];
        const tinygltf::BufferView& bufferView =
            gltfModel.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

        indexCount = static_cast<uint32_t>(accessor.count);
        const void* dataPtr =
            &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

        // adjust index vector size to store proper number of indices
        indices.resize(indexCount);

        switch (accessor.componentType) {
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
            const uint32_t* pBuffer = static_cast<const uint32_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
              indices[index + indexStart] = pBuffer[index] + vertexStart;
            }
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
            const uint16_t* pBuffer = static_cast<const uint16_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
              indices[index + indexStart] = pBuffer[index] + vertexStart;
            }
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
            const uint8_t* pBuffer = static_cast<const uint8_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
              indices[index + indexStart] = pBuffer[index] + vertexStart;
            }
            break;
          }
          default:
            RE_LOG(Error, "index component type %d not supported",
                   accessor.componentType);
            return;
        }
      }

      // create new primitive for storing vertex and index data
      pMesh->pPrimitives.emplace_back(
          std::make_unique<WPrimitive_Custom>(vertices, indices));
      WPrimitive* pPrimitive = pMesh->pPrimitives.back().get();
      pPrimitive->setBoundingBoxExtent(posMin, posMax);
    }

    // calculate bounding box extent for the whole mesh based on created
    // primitives
    glm::vec3 minExtent{0.0f}, maxExtent{0.0f};
    for (const auto& primitive : pMesh->pPrimitives) {
      if (primitive->getBoundingBoxExtent(minExtent, maxExtent)) {
        pMesh->extent.min = glm::min(pMesh->extent.min, minExtent);
        pMesh->extent.max = glm::max(pMesh->extent.max, maxExtent);
        pMesh->extent.isValid = true;

        m_pLinearPrimitives.emplace_back(primitive.get());
      }
    }
  }

  // add a node for linear access
  m_pLinearNodes.emplace_back(pNode);
}

WModel::Node* WModel::createNode(WModel::Node* pParentNode, uint32_t nodeIndex,
                                 std::string nodeName) {
  WModel::Node* pNode = nullptr;

  if (pParentNode == nullptr) {
    m_pChildNodes.emplace_back(std::make_unique<WModel::Node>(
        pParentNode, nodeIndex, nodeName));
    pNode = m_pChildNodes.back().get();
  } else {
    pParentNode->pChildren.emplace_back(std::make_unique<WModel::Node>(
        pParentNode, nodeIndex, nodeName));
    pNode = pParentNode->pChildren.back().get();
  }

  if (!pNode) {
    RE_LOG(Error, "Trying to create the node '%s', but got nullptr.",
           nodeName.c_str());
    return nullptr;
  }

  pNode->name = nodeName;
  pNode->nodeMatrix = glm::mat4(1.0f);

  pNode->pMesh = std::make_unique<WModel::Mesh>();

  return pNode;
}

void WModel::destroyNode(std::unique_ptr<WModel::Node>& pNode) {
  if (!pNode->pChildren.empty()) {
    for (auto& pChildNode : pNode->pChildren) {
      destroyNode(pChildNode);
    };
  }

  if (pNode->pMesh) {
    auto& primitives = pNode->pMesh->pPrimitives;
    for (auto& primitive : primitives) {
      if (primitive) {
        primitive->destroy();
        primitive.reset();
      }
    }

    pNode->pChildren.clear();
    pNode->pMesh->pPrimitives.clear();
    pNode->pMesh.reset();

    vmaFreeMemory(core::renderer.memAlloc,
                  pNode->uniformBufferData.uniformBuffer.allocation);
  }

  pNode.reset();
}

bool WModel::Mesh::validateBoundingBoxExtent() {
  if (extent.min == extent.max) {
    extent.isValid = false;
    RE_LOG(Warning, "Bounding box of a mesh was invalidated.");
  }

  return extent.isValid;
}

const std::vector<WPrimitive*>& WModel::getPrimitives() {
  return m_pLinearPrimitives;
}

TResult WModel::clean() {
  if (m_pChildNodes.empty()) {
    RE_LOG(Warning,
           "Model '%s' can not be cleared. It may be already empty and ready "
           "for deletion.",
           m_name.c_str());

    return RE_WARNING;
  }

  for (auto& node : m_pChildNodes) {
    destroyNode(node);
  }

  m_pChildNodes.clear();
  m_pLinearNodes.clear();
  m_pLinearPrimitives.clear();

  RE_LOG(Log, "Model '%s' is prepared for deletion.", m_name.c_str());

  return RE_OK;
}
