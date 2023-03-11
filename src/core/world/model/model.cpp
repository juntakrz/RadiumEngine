#include "pch.h"
#include "core/core.h"
#include "core/managers/materials.h"
#include "core/world/model/model.h"

#include "tiny_gltf.h"

void WModel::setTextureSamplers(const tinygltf::Model& gltfModel) {
  auto getVkFilter = [](const int& filterMode) {
    switch (filterMode) {
      case -1:
      case 9728:
      case 9984:
      case 9985:
        return VK_FILTER_NEAREST;
      case 9729:
      case 9986:
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

void WModel::parseMaterials(const tinygltf::Model& gltfModel,
                            const std::vector<std::string>& texturePaths) {

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
    core::materials.createMaterial(&materialInfo);
    m_materialList.emplace_back(materialInfo.name);
  }
}

void WModel::loadAnimations(const tinygltf::Model& gltfModel) {
  for (const tinygltf::Animation& anim : gltfModel.animations) {
    Animation animation{};
    animation.name = anim.name;
    if (anim.name.empty()) {
      animation.name = m_name + "_" + std::to_string(m_animations.size());
    }

    // Samplers
    for (auto& samp : anim.samplers) {
      AnimationSampler sampler{};

      if (samp.interpolation == "LINEAR") {
        sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
      }
      if (samp.interpolation == "STEP") {
        sampler.interpolation = AnimationSampler::InterpolationType::STEP;
      }
      if (samp.interpolation == "CUBICSPLINE") {
        sampler.interpolation =
            AnimationSampler::InterpolationType::CUBICSPLINE;
      }

      // Read sampler input time values
      {
        const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
        const tinygltf::BufferView& bufferView =
            gltfModel.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        const void* dataPtr =
            &buffer.data[accessor.byteOffset + bufferView.byteOffset];
        const float* buf = static_cast<const float*>(dataPtr);
        for (size_t index = 0; index < accessor.count; index++) {
          sampler.inputs.emplace_back(buf[index]);
        }

        for (auto input : sampler.inputs) {
          if (input < animation.start) {
            animation.start = input;
          };
          if (input > animation.end) {
            animation.end = input;
          }
        }
      }

      // Read sampler output T/R/S values
      {
        const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
        const tinygltf::BufferView& bufferView =
            gltfModel.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        const void* dataPtr =
            &buffer.data[accessor.byteOffset + bufferView.byteOffset];

        switch (accessor.type) {
          case TINYGLTF_TYPE_VEC3: {
            const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
              sampler.outputsVec4.emplace_back(glm::vec4(buf[index], 0.0f));
            }
            break;
          }
          case TINYGLTF_TYPE_VEC4: {
            const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
              sampler.outputsVec4.emplace_back(buf[index]);
            }
            break;
          }
          default: {
            std::cout << "unknown type" << std::endl;
            break;
          }
        }
      }

      animation.samplers.emplace_back(sampler);
    }

    // Channels
    for (auto& source : anim.channels) {
      AnimationChannel channel{};

      if (source.target_path == "rotation") {
        channel.path = AnimationChannel::PathType::ROTATION;
      }
      if (source.target_path == "translation") {
        channel.path = AnimationChannel::PathType::TRANSLATION;
      }
      if (source.target_path == "scale") {
        channel.path = AnimationChannel::PathType::SCALE;
      }
      if (source.target_path == "weights") {
        std::cout << "weights not yet supported, skipping channel" << std::endl;
        continue;
      }
      channel.samplerIndex = source.sampler;
      channel.node = getNode(source.target_node);
      if (!channel.node) {
        continue;
      }

      animation.channels.emplace_back(channel);
    }

    m_animations.emplace_back(animation);
  }
}

void WModel::loadSkins(const tinygltf::Model& gltfModel) {
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

const char* WModel::getName() { return m_name.c_str(); }

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

std::vector<uint32_t>& WModel::getPrimitiveBindsIndex() {
  return m_primitiveBindsIndex;
}

const std::vector<std::unique_ptr<WModel::Node>>& WModel::getRootNodes() noexcept {
  return m_pChildNodes;
}

std::vector<WModel::Node*>& WModel::getAllNodes() noexcept {
  return m_pLinearNodes;
}

TResult WModel::validateStagingBuffers() {
  if (m_indexCount != m_currentIndexOffset ||
      m_vertexCount != m_currentVertexOffset ||
      m_indexStaging.size() != m_indexCount ||
      m_vertexStaging.size() != m_vertexCount) {
    return RE_ERROR;
  }
  return RE_OK;
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

  for (auto& skin : m_pSkins) {
    skin.reset();
  }
  m_pSkins.clear();

  m_pChildNodes.clear();
  m_pLinearNodes.clear();
  m_pLinearPrimitives.clear();

  RE_LOG(Log, "Model '%s' is prepared for deletion.", m_name.c_str());

  return RE_OK;
}

void WModel::setLocalStagingBuffers() {
  if (m_vertexCount == 0 || m_indexCount == 0) {
    RE_LOG(Error,
           "Can't prepare local staging buffers for model \"%s\", nodes "
           "weren't parsed yet.");
    return;
  }

  m_vertexStaging.resize(m_vertexCount);
  m_indexStaging.resize(m_indexCount);
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