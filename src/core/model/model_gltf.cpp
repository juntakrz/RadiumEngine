#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/animations.h"
#include "core/model/model.h"

#include "tiny_gltf.h"

TResult WModel::createModel(const char* name, const tinygltf::Model* pInModel,
                            const WModelConfigInfo* pConfigInfo) {
  if (!pInModel) {
    RE_LOG(Error,
           "Failed to create model, no glTF source was provided for \"%s\".",
           name);
  }

  staging.pInModel = pInModel;
  const tinygltf::Model& gltfModel = *pInModel;

  m_name = name;

  uint32_t vertexCount = 0u, vertexPos = 0u, indexCount = 0u, indexPos = 0u;
  const tinygltf::Scene& gltfScene =
      gltfModel
          .scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

  // index of texture paths used by this model, required by the material setup
  // later
  std::vector<std::string> texturePaths;

  // assign texture samplers for the current WModel
  setTextureSamplers();

  // go through glTF model's texture records
  for (const tinygltf::Texture& tex : gltfModel.textures) {
    const tinygltf::Image* pImage = &gltfModel.images[tex.source];

    // get custom corresponding sampler data from WModel if present
    RSamplerInfo textureSampler{};
    if (tex.sampler != -1) {
      textureSampler = m_textureSamplers[tex.sampler];
    }

    // add empty path, should be changed later
    texturePaths.emplace_back("");

    // get texture name and load it though materials manager
    if (pImage->uri == "") {
      continue;
    }

#ifndef NDEBUG
    RE_LOG(Log, "Loading texture \"%s\" for model \"%s\".", pImage->uri.c_str(),
           m_name.c_str());
#endif
    if (core::resources.loadTexture(pImage->uri.c_str(), &textureSampler) <
        RE_ERROR) {
      texturePaths.back() = pImage->uri;
    };
  }

  // get glTF materials and convert them to RMaterial
  parseMaterials(texturePaths);

  // parse node properties and get index/vertex counts
  for (size_t i = 0; i < gltfScene.nodes.size(); ++i) {
    parseNodeProperties(gltfModel.nodes[gltfScene.nodes[i]]);
  }

  // resize local staging buffers
  prepareStagingData();

  // create nodes using data from glTF model
  for (size_t n = 0; n < gltfScene.nodes.size(); ++n) {
    const tinygltf::Node& gltfNode = gltfModel.nodes[gltfScene.nodes[n]];
    createNode(nullptr, gltfNode, gltfScene.nodes[n]);
  }

  // validate model staging buffers
  if (validateStagingData() != RE_OK) {
    RE_LOG(Error,
           "Failed to create model \"%s\". Error when generating buffers.",
           m_name.c_str());
    return RE_ERROR;
  }

  loadSkins();

  for (auto node : m_pLinearNodes) {
    // Assign skins
    if (node->skinIndex > -1) {
      node->pSkin = m_pSkins[node->skinIndex].get();

      // set joint count for every instance of a skin
      if (node->pSkin) {
        size_t jointCount =
            std::min((uint32_t)node->pSkin->joints.size(), RE_MAXJOINTS);
        node->pMesh->uniformBlock.jointCount = (float)jointCount;
      }
    }

    // Initial pose
    updateNodeTransformBuffer();
  }

  if (pConfigInfo &&
      pConfigInfo->animationLoadMode > EAnimationLoadMode::OnDemand) {
    if (gltfModel.animations.size() > 0) {
      extractAnimations(pConfigInfo);
    }
  }

  sortPrimitivesByMaterial();

  resetUniformBlockData();

  return createStagingBuffers();
}

void WModel::createNode(WModel::Node* pParentNode,
                        const tinygltf::Node& gltfNode,
                        uint32_t gltfNodeIndex) {
  WModel::Node* pNode = nullptr;
  const tinygltf::Model& gltfModel = *staging.pInModel;

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
  pNode->staging.nodeMatrix = glm::mat4(1.0f);

  // general local node matrix
  if (gltfNode.translation.size() == 3) {
    pNode->staging.translation = glm::make_vec3(gltfNode.translation.data());
  }

  if (gltfNode.rotation.size() == 4) {
    glm::quat q = glm::make_quat(gltfNode.rotation.data());
    pNode->staging.rotation = glm::mat4(q);   // why if both are quaternions? check later
  }

  if (gltfNode.scale.size() == 3) {
    pNode->staging.scale = glm::make_vec3(gltfNode.scale.data());
  }

  if (gltfNode.matrix.size() == 14) {
    pNode->staging.nodeMatrix = glm::make_mat4x4(gltfNode.matrix.data());
  }

  // recursively create node children
  if (gltfNode.children.size() > 0) {
    for (size_t i = 0; i < gltfNode.children.size(); ++i) {
      createNode(pNode, gltfModel.nodes[gltfNode.children[i]],
                 gltfNode.children[i]);
    }
  }

  // retrieve mesh data if available
  if (gltfNode.mesh > -1) {
    const tinygltf::Mesh& gltfMesh = gltfModel.meshes[gltfNode.mesh];
    pNode->pMesh = std::make_unique<WModel::Mesh>();
    // allocate buffer for UBO used for writing mesh transformation data
    pNode->allocateMeshBuffer();
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

        // position
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

          if (core::vulkan::applyGLTFLeftHandedFix) {
            vertex.pos.x = -vertex.pos.x;
            vertex.normal.x = -vertex.normal.x;
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

        if (core::vulkan::applyGLTFLeftHandedFix) {
          for (auto it = indices.begin(); it != indices.end(); it += 3) {
            std::swap(*it, *(it + 2));
          }
        }
      }

      RPrimitiveInfo primitiveInfo{};
      primitiveInfo.vertexOffset = staging.currentVertexOffset;
      primitiveInfo.indexOffset = staging.currentIndexOffset;
      primitiveInfo.vertexCount = static_cast<uint32_t>(vertices.size());
      primitiveInfo.indexCount = static_cast<uint32_t>(indices.size());

      primitiveInfo.createTangentSpaceData = true;
      primitiveInfo.pVertexData = &vertices;
      primitiveInfo.pIndexData = &indices;
      primitiveInfo.pOwnerNode = pNode;

      // create new primitive
      pMesh->pPrimitives.emplace_back(
          std::make_unique<WPrimitive>(&primitiveInfo));
      WPrimitive* pPrimitive = pMesh->pPrimitives.back().get();
      pPrimitive->setBoundingBoxExtent(posMin, posMax);
      pPrimitive->pMaterial = core::resources.getMaterial(
          m_materialList[gltfPrimitive.material].c_str());

      // copy vertex and index data to local staging buffers and adjust offsets
      std::copy(vertices.begin(), vertices.end(),
                staging.vertices.begin() + staging.currentVertexOffset);
      std::copy(indices.begin(), indices.end(),
                staging.indices.begin() + staging.currentIndexOffset);

      staging.currentVertexOffset += primitiveInfo.vertexCount;
      staging.currentIndexOffset += primitiveInfo.indexCount;
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

void WModel::parseNodeProperties(const tinygltf::Node& gltfNode) {
  const tinygltf::Model& gltfModel = *staging.pInModel;

  if (gltfNode.children.size() > 0) {
    for (size_t i = 0; i < gltfNode.children.size(); i++) {
      parseNodeProperties(gltfModel.nodes[gltfNode.children[i]]);
    }
  }

  if (gltfNode.mesh > -1) {
    const tinygltf::Mesh& mesh = gltfModel.meshes[gltfNode.mesh];

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
      const tinygltf::Primitive& currentPrimitive = mesh.primitives[i];

      // get vertex count of a current primitive
      m_vertexCount += static_cast<uint32_t>(
          gltfModel
              .accessors[currentPrimitive.attributes.find("POSITION")->second]
              .count);

      // get index count of a current primitive
      if (currentPrimitive.indices > -1) {
        m_indexCount += static_cast<uint32_t>(
            gltfModel.accessors[currentPrimitive.indices].count);
      }
    }
  }
}

void WModel::setTextureSamplers() {
  auto getVkFilter = [](const int& filterMode) {
    switch (filterMode) {
      case -1:
      case 9728:
        return VK_FILTER_NEAREST;
      case 9729:
        return VK_FILTER_LINEAR;
      case 9984:
        return VK_FILTER_NEAREST;
      case 9985:
        return VK_FILTER_NEAREST;
      case 9986:
        return VK_FILTER_LINEAR;
      case 9987:
        return VK_FILTER_LINEAR;
    }
    RE_LOG(Warning,
           "Unknown filter mode %d for getVkFilter. Using VK_FILTER_NEAREST.",
           filterMode);
    return VK_FILTER_NEAREST;
  };

  auto getVkAddressMode = [](const int& addressMode) {
    switch (addressMode) {
      case -1:
      case 10497:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
      case 33071:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      case 33648:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }

    RE_LOG(Warning,
           "Unknown wrap mode %d for getVkAddressMode. Using "
           "VK_SAMPLER_ADDRESS_MODE_REPEAT.",
           addressMode);
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  };

  const tinygltf::Model& gltfModel = *staging.pInModel;

  for (const auto& it : gltfModel.samplers) {
    m_textureSamplers.emplace_back();
    auto& sampler = m_textureSamplers.back();
    sampler.minFilter = getVkFilter(it.minFilter);
    sampler.magFilter = getVkFilter(it.magFilter);
    sampler.addressModeU = getVkAddressMode(it.wrapS);
    sampler.addressModeV = getVkAddressMode(it.wrapT);
    sampler.addressModeW = sampler.addressModeV;
  }
}

void WModel::parseMaterials(const std::vector<std::string>& texturePaths) {
  const tinygltf::Model& gltfModel = *staging.pInModel;

  for (const tinygltf::Material& mat : gltfModel.materials) {
    RMaterialInfo materialInfo{};

    materialInfo.doubleSided = mat.doubleSided;

    if (mat.name == "") {
      RE_LOG(
          Error,
          "Failed to retrieve material from the model, material name is empty");
      return;
    }

    materialInfo.name = m_name + "_" + mat.name;

    if (mat.values.contains("baseColorTexture")) {
      materialInfo.textures.baseColor =
          texturePaths[mat.values.at("baseColorTexture").TextureIndex()];
      materialInfo.texCoordSets.baseColor =
          mat.values.at("baseColorTexture").TextureTexCoord();
    }

    if (mat.additionalValues.contains("normalTexture")) {
      materialInfo.textures.normal =
          texturePaths[mat.additionalValues.at("normalTexture").TextureIndex()];
      materialInfo.texCoordSets.normal =
          mat.additionalValues.at("normalTexture").TextureTexCoord();
    }

    if (mat.values.contains("metallicRoughnessTexture")) {
      materialInfo.textures.metalRoughness =
          texturePaths[mat.values.at("metallicRoughnessTexture")
                           .TextureIndex()];
      materialInfo.texCoordSets.metalRoughness =
          mat.values.at("metallicRoughnessTexture").TextureTexCoord();
    }

    if (mat.additionalValues.contains("occlusionTexture")) {
      materialInfo.textures.occlusion =
          texturePaths[mat.additionalValues.at("occlusionTexture")
                           .TextureIndex()];
      materialInfo.texCoordSets.occlusion =
          mat.additionalValues.at("occlusionTexture").TextureTexCoord();
    }
    if (mat.additionalValues.contains("emissiveTexture")) {
      materialInfo.textures.emissive =
          texturePaths[mat.additionalValues.at("emissiveTexture")
                           .TextureIndex()];
      materialInfo.texCoordSets.emissive =
          mat.additionalValues.at("emissiveTexture").TextureTexCoord();
    }

    if (mat.values.contains("baseColorFactor")) {
      materialInfo.baseColorFactor =
          glm::make_vec4(mat.values.at("baseColorFactor").ColorFactor().data());
    }

    if (mat.values.contains("metallicFactor")) {
      materialInfo.metallicFactor =
          static_cast<float>(mat.values.at("metallicFactor").Factor());
    }

    if (mat.values.contains("roughnessFactor")) {
      materialInfo.roughnessFactor =
          static_cast<float>(mat.values.at("roughnessFactor").Factor());
    }

    if (mat.additionalValues.contains("emissiveFactor")) {
      materialInfo.emissiveFactor = glm::vec4(
          glm::make_vec3(
              mat.additionalValues.at("emissiveFactor").ColorFactor().data()),
          1.0);
    }

    if (mat.additionalValues.contains("alphaMode")) {
      tinygltf::Parameter param = mat.additionalValues.at("alphaMode");

      if (param.string_value == "BLEND") {
        materialInfo.alphaMode = EAlphaMode::Blend;
      }

      if (param.string_value == "MASK") {
        materialInfo.alphaCutoff = 0.5f;
        materialInfo.alphaMode = EAlphaMode::Mask;
      }
    }

    if (mat.additionalValues.contains("alphaCutoff")) {
      materialInfo.alphaCutoff =
          static_cast<float>(mat.additionalValues.at("alphaCutoff").Factor());
    }

    // create new material
    core::resources.createMaterial(&materialInfo);
    m_materialList.emplace_back(materialInfo.name);
  }
}

void WModel::extractAnimations(const WModelConfigInfo* pConfigInfo) {
  if (!pConfigInfo) {
    RE_LOG(Error,
           "Unable to extract animations from model '%s', the required "
           "information is missing.",
           m_name.c_str());

    return;
  }

  const tinygltf::Model& gltfModel = *staging.pInModel;

  for (int32_t i = 0; i < gltfModel.animations.size(); ++i) {
    const tinygltf::Animation& gltfAnimation = gltfModel.animations[i];
    std::string animationName;
    WAnimation* pAnimation = nullptr;
    const size_t animationDataEntries = gltfAnimation.samplers.size();

    float startTime = std::numeric_limits<float>::max();
    float endTime = std::numeric_limits<float>::min();

    if (gltfAnimation.name.empty()) {
      animationName = m_name + "_" + std::to_string(i);
    } else {
      animationName = gltfAnimation.name;
    }

    pAnimation = core::animations.createAnimation(animationName);

    // skip adding animation if a similarly named one already exists
    if (!pAnimation) {
      continue;
    }

    // staging transform block containing node id and all its frames
    auto& stagingTransformData = pAnimation->getStagingTransformData();

    for (int32_t j = 0; j < animationDataEntries; ++j) {
      const tinygltf::AnimationSampler& gltfSampler = gltfAnimation.samplers[j];
      const tinygltf::AnimationChannel& gltfChannel = gltfAnimation.channels[j];

      const tinygltf::Accessor& timeAccessor =
          gltfModel.accessors[gltfSampler.input];
      const tinygltf::Accessor& transformAccessor =
          gltfModel.accessors[gltfSampler.output];

      const tinygltf::BufferView& timeBufferView =
          gltfModel.bufferViews[timeAccessor.bufferView];
      const tinygltf::Buffer& timeBuffer =
          gltfModel.buffers[timeBufferView.buffer];

      const tinygltf::BufferView& transformBufferView =
          gltfModel.bufferViews[transformAccessor.bufferView];
      const tinygltf::Buffer& transformBuffer =
          gltfModel.buffers[transformBufferView.buffer];

      const size_t frames = timeAccessor.count;
      const int32_t nodeIndex = gltfChannel.target_node;

      // create an entry in staging transform data
      auto& stagingTransformBlock = stagingTransformData.emplace_back();
      stagingTransformBlock.nodeIndex = nodeIndex;
      stagingTransformBlock.frameData.resize(frames);

      // time and transformation entries should always be float
      assert(timeAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
      assert(transformAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

      const void* pTimeData =
          &timeBuffer.data[timeAccessor.byteOffset + timeBufferView.byteOffset];
      const float* pTimeDataView = static_cast<const float*>(pTimeData);

      for (size_t index = 0; index < frames; ++index) {
        auto& StagingTransformFrameEntry =
            stagingTransformData.back().frameData.at(index);
        StagingTransformFrameEntry.timeStamp = pTimeDataView[index];

        // update start and end time of this animation
        if (StagingTransformFrameEntry.timeStamp < startTime) {
          startTime = StagingTransformFrameEntry.timeStamp;
        };
        if (StagingTransformFrameEntry.timeStamp > endTime) {
          endTime = StagingTransformFrameEntry.timeStamp;
        }

        if (gltfChannel.target_path == "translation") {
          StagingTransformFrameEntry.transformType =
              ETransformType::Translation;
        } else if (gltfChannel.target_path == "rotation") {
          StagingTransformFrameEntry.transformType = ETransformType::Rotation;
        } else if (gltfChannel.target_path == "scale") {
          StagingTransformFrameEntry.transformType = ETransformType::Scale;
        } else if (gltfChannel.target_path == "weights") {
          StagingTransformFrameEntry.transformType = ETransformType::Weight;
        }

        // currently only translation, rotation and scale are supported
        if (StagingTransformFrameEntry.transformType > ETransformType::Scale) {
          RE_LOG(Error,
                 "Unsupported transform type for animation '%s', "
                 "animation data entry: %d, node index: %d, frame: %d.",
                 animationName.c_str(), j, nodeIndex, index);
          continue;
        }

        const void* pTransformData =
            &transformBuffer.data[transformAccessor.byteOffset +
                                  transformBufferView.byteOffset];

        switch (transformAccessor.type) {
          case TINYGLTF_TYPE_VEC3: {
            // can't use glm::vec3 to iterate due to enforced 16 byte alignment
            // so need to retrieve data by using single value address step
            const float* transformDataView =
                static_cast<const float*>(pTransformData);

            // align data view by 3 values / 12 bytes
            size_t viewAddress = index * 3;

            // store data into the transformation vec4 of an appropriate type
            StagingTransformFrameEntry.transformData =
                glm::vec4(transformDataView[viewAddress],
                          transformDataView[viewAddress + 1],
                          transformDataView[viewAddress + 2], 0.0f);
            break;
          }
          case TINYGLTF_TYPE_VEC4: {
            const glm::vec4* transformDataView =
                static_cast<const glm::vec4*>(pTransformData);

            StagingTransformFrameEntry.transformData = transformDataView[index];
            break;
          }
          default: {
            RE_LOG(Warning,
                   "Unknown or unsupported sampler accessor type %d for model "
                   "'%s'.",
                   transformAccessor.type, m_name.c_str());
            break;
          }
        }
      }

      // set time range of the animation
      pAnimation->setStagingTimeRange(startTime, endTime);
    }

    pAnimation->resampleKeyFrames(this, pConfigInfo->framerate,
                                  pConfigInfo->speed);
    pAnimation->clearStagingTransformData();

    if (pConfigInfo->animationLoadMode >
        EAnimationLoadMode::ExtractToManagerAndStorage) {
      core::animations.saveAnimation(
          pAnimation->getName(), pAnimation->getName(), pConfigInfo->skeleton);

      if (pConfigInfo->animationLoadMode ==
          EAnimationLoadMode::ExtractToStorageOnly) {
        core::animations.removeAnimation(pAnimation->getName());
      }
    }
  }
}

void WModel::loadSkins() {
  const tinygltf::Model& gltfModel = *staging.pInModel;

  for (const tinygltf::Skin& source : gltfModel.skins) {
    m_pSkins.emplace_back(std::make_unique<Skin>());
    Skin* pSkin = m_pSkins.back().get();
    pSkin->name = source.name;

    // Find skeleton root node
    if (source.skeleton > -1) {
      pSkin->skeletonRoot = getNode(source.skeleton);
    }

    // Find joint nodes
    for (int jointIndex : source.joints) {
      Node* pNode = getNode(jointIndex);
      if (pNode) {
        pSkin->joints.emplace_back(pNode);
      }
    }

    // Get inverse bind matrices from buffer
    if (source.inverseBindMatrices > -1) {
      const tinygltf::Accessor& accessor =
          gltfModel.accessors[source.inverseBindMatrices];
      const tinygltf::BufferView& bufferView =
          gltfModel.bufferViews[accessor.bufferView];
      const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
      pSkin->inverseBindMatrices.resize(accessor.count);
      memcpy(pSkin->inverseBindMatrices.data(),
             &buffer.data[accessor.byteOffset + bufferView.byteOffset],
             accessor.count * sizeof(glm::mat4));
    }
  }
}
