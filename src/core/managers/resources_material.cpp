#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/resources.h"

core::MResources::MResources() {
  RE_LOG(Log, "Setting up the materials manager.");
}

void core::MResources::initialize() {
  RE_LOG(Log, "Initializing materials manager data.");

  RMaterial* pMaterial = nullptr;
  m_materialIndices.resize(config::scene::sampledImageBudget / RE_MAXTEXTURES, nullptr);
  m_samplerIndices.resize(config::scene::sampledImageBudget, nullptr);

  // create the "default" material
  RSamplerInfo samplerInfo{};

  loadTexture(RE_DEFAULTTEXTURE, &samplerInfo);
  loadTexture("default/default_normal.ktx2", &samplerInfo);
  loadTexture("default/default_metallicRoughness.ktx2", &samplerInfo);
  loadTexture("default/default_occlusion.ktx2", &samplerInfo);
  loadTexture(RE_BLACKTEXTURE, &samplerInfo);
  loadTexture(RE_WHITETEXTURE, &samplerInfo);

  // Create default material
  RMaterialInfo materialInfo{};
  materialInfo.name = "default";
  materialInfo.passFlags = EDynamicRenderingPass::OpaqueCullBack;
  materialInfo.textures.baseColor = RE_DEFAULTTEXTURE;

  createMaterial(&materialInfo);

  materialInfo = RMaterialInfo{};
  materialInfo.name = RMAT_SHADOW;
  materialInfo.textures.baseColor = RTGT_SHADOW;
  materialInfo.doubleSided = false;
  materialInfo.manageTextures = true;
  materialInfo.passFlags = EDynamicRenderingPass::Shadow;

  if (!(pMaterial = createMaterial(&materialInfo))) {
    RE_LOG(Critical, "Failed to create directional sun shadow material.");

    return;
  }

  core::renderer.getMaterialData()->pSunShadow = pMaterial;
  core::renderer.getLightingData()->data.samplerArrayIndex[0] = pMaterial->pBaseColor->combinedSamplerIndex;

  materialInfo = RMaterialInfo{};
  materialInfo.name = RMAT_GBUFFER;
  materialInfo.textures.baseColor = RTGT_GDIFFUSE;
  materialInfo.textures.normal = RTGT_GNORMAL;
  materialInfo.textures.metalRoughness = RTGT_GPHYSICAL;
  materialInfo.textures.occlusion = RTGT_GPOSITION;
  materialInfo.textures.emissive = RTGT_GEMISSIVE;
  materialInfo.alphaMode = EAlphaMode::Opaque;
  materialInfo.doubleSided = false;
  materialInfo.manageTextures = true;
  materialInfo.passFlags = EDynamicRenderingPass::PBR;

  if (!(pMaterial = createMaterial(&materialInfo))) {
    RE_LOG(Critical, "Failed to create Vulkan G-Buffer material.");

    return;
  }

  core::renderer.getMaterialData()->pGBuffer = pMaterial;

  // Create present material that takes combined output of all render passes as
  // a shader read only attachment
  materialInfo = RMaterialInfo{};
  materialInfo.name = RMAT_MAIN;
  materialInfo.textures.baseColor = RTGT_GPBR;
  materialInfo.textures.normal = RTGT_PPBLOOM;
  materialInfo.textures.metalRoughness = RTGT_VELOCITYMAP;
  materialInfo.textures.occlusion = RTGT_PREVFRAME;
  materialInfo.textures.emissive = RTGT_PPTAA;
  materialInfo.textures.extra0 = RTGT_APBR;
  materialInfo.textures.extra1 = RTGT_PPAO;
  materialInfo.alphaMode = EAlphaMode::Opaque;
  materialInfo.doubleSided = false;
  materialInfo.manageTextures = true;
  materialInfo.passFlags = EDynamicRenderingPass::Present;

  if (!(pMaterial = createMaterial(&materialInfo))) {
    RE_LOG(Critical, "Failed to create Vulkan present material.");

    return;
  }

  core::renderer.getMaterialData()->pGPBR = pMaterial;

  // Create present material that takes combined output of all render passes as
  // a shader read only attachment
  materialInfo = RMaterialInfo{};
  materialInfo.name = RMAT_BLUR;
  materialInfo.textures.baseColor = RTGT_PPBLUR;
  materialInfo.textures.occlusion = RTGT_PPAO;
  materialInfo.alphaMode = EAlphaMode::Opaque;
  materialInfo.doubleSided = false;
  materialInfo.manageTextures = true;
  materialInfo.passFlags = EDynamicRenderingPass::PPBlur;

  if (!(pMaterial = createMaterial(&materialInfo))) {
    RE_LOG(Critical, "Failed to create Vulkan present material.");

    return;
  }

  core::renderer.getMaterialData()->pBlur = pMaterial;
}

uint32_t core::MResources::getFreeCombinedSamplerIndex() {
  for (uint32_t i = 0; i < m_samplerIndices.size(); ++i) {
    if (m_samplerIndices[i] == nullptr) return i;
  }

  RE_LOG(Warning, "No more free sampler2D descriptor entries left.");
  return -1;
}

void core::MResources::updateMaterialDescriptorSet(RTexture* pTexture, EResourceType resourceType) {
  // Don't create duplicate entries if this texture was already written to descriptor set
  if (pTexture->references > 0) return;

  VkWriteDescriptorSet writeSet{};
  writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSet.dstSet = core::renderer.getMaterialDescriptorSet();
  writeSet.descriptorCount = 1;

  switch (resourceType) {
    case EResourceType::Sampler2D: {
      uint32_t index = getFreeCombinedSamplerIndex();

      VkDescriptorImageInfo imageInfo = pTexture->texture.imageInfo;

      // HACK: this image is expected to be translated to a proper layout later
      switch (imageInfo.imageLayout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
          imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          break;
        }
      }

      writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeSet.dstBinding = 1;
      writeSet.dstArrayElement = index;
      writeSet.pImageInfo = &imageInfo;

      vkUpdateDescriptorSets(core::renderer.logicalDevice.device, 1u, &writeSet, 0, nullptr);

      // Store index data
      pTexture->combinedSamplerIndex = index;
      m_samplerIndices[index] = pTexture;

      return;
    }
    default: {
      return;
    }
  }
}

RMaterial* core::MResources::createMaterial(
    RMaterialInfo* pDesc) noexcept {

  if (!m_materials.try_emplace(pDesc->name).second) {
#ifndef NDEBUG
    RE_LOG(Warning, "Material \"%s\" is already created.", pDesc->name.c_str());
#endif
    return m_materials.at(pDesc->name).get();
  }

  // Storing index 0 as 0b10 and 1 as 0b11 to save precious push block space
  // and for a reliable bitwise retrieval in the shader
  auto fGetTextureIndex = [](const int8_t textureSet, const int8_t textureIndex) {
    return (textureSet + 2) << (textureIndex * 2);
  };

  RMaterial newMat;
  newMat.name = pDesc->name;

  newMat.pBaseColor = assignTexture(pDesc->textures.baseColor.c_str());
  newMat.pNormal = assignTexture(pDesc->textures.normal.c_str());
  newMat.pMetalRoughness = assignTexture(pDesc->textures.metalRoughness.c_str());
  newMat.pOcclusion = assignTexture(pDesc->textures.occlusion.c_str());
  newMat.pEmissive = assignTexture(pDesc->textures.emissive.c_str());
  newMat.pExtra0 = assignTexture(pDesc->textures.extra0.c_str());
  newMat.pExtra1 = assignTexture(pDesc->textures.extra1.c_str());
  newMat.pExtra2 = assignTexture(pDesc->textures.extra2.c_str());

  newMat.pushConstantBlock.metallicFactor = pDesc->metallicFactor;
  newMat.pushConstantBlock.roughnessFactor = pDesc->roughnessFactor;
  newMat.pushConstantBlock.alphaMode = static_cast<float>(pDesc->alphaMode);
  newMat.pushConstantBlock.alphaCutoff = pDesc->alphaCutoff;
  newMat.pushConstantBlock.bumpIntensity = pDesc->bumpIntensity;
  newMat.pushConstantBlock.glowColor = pDesc->glowColor;

  // disable reading from texture in shader if no texture is available
  newMat.pushConstantBlock.textureSets |= newMat.pBaseColor ? fGetTextureIndex(pDesc->texCoordSets.baseColor, 0) : 0;
  newMat.pushConstantBlock.textureSets |= newMat.pNormal ? fGetTextureIndex(pDesc->texCoordSets.normal, 1) : 0;
  newMat.pushConstantBlock.textureSets |= newMat.pMetalRoughness ? fGetTextureIndex(pDesc->texCoordSets.metalRoughness, 2) : 0;
  newMat.pushConstantBlock.textureSets |= newMat.pOcclusion ? fGetTextureIndex(pDesc->texCoordSets.occlusion, 3) : 0;
  newMat.pushConstantBlock.textureSets |= newMat.pEmissive ? fGetTextureIndex(pDesc->texCoordSets.emissive, 4) : 0;
  newMat.pushConstantBlock.textureSets |= newMat.pExtra0 ? fGetTextureIndex(pDesc->texCoordSets.extra0, 5) : 0;
  newMat.pushConstantBlock.textureSets |= newMat.pExtra1 ? fGetTextureIndex(pDesc->texCoordSets.extra1, 6) : 0;
  newMat.pushConstantBlock.textureSets |= newMat.pExtra2 ? fGetTextureIndex(pDesc->texCoordSets.extra2, 7) : 0;

  newMat.pushConstantBlock.samplerIndex[0] = newMat.pBaseColor ? newMat.pBaseColor->combinedSamplerIndex : 0;
  newMat.pushConstantBlock.samplerIndex[1] = newMat.pNormal ? newMat.pNormal->combinedSamplerIndex : 0;
  newMat.pushConstantBlock.samplerIndex[2] = newMat.pMetalRoughness ? newMat.pMetalRoughness->combinedSamplerIndex : 0;
  newMat.pushConstantBlock.samplerIndex[3] = newMat.pOcclusion ? newMat.pOcclusion->combinedSamplerIndex : 0;
  newMat.pushConstantBlock.samplerIndex[4] = newMat.pEmissive ? newMat.pEmissive->combinedSamplerIndex : 0;
  newMat.pushConstantBlock.samplerIndex[5] = newMat.pExtra0 ? newMat.pExtra0->combinedSamplerIndex : 0;
  newMat.pushConstantBlock.samplerIndex[6] = newMat.pExtra1 ? newMat.pExtra1->combinedSamplerIndex : 0;
  newMat.pushConstantBlock.samplerIndex[7] = newMat.pExtra2 ? newMat.pExtra2->combinedSamplerIndex : 0;

  // store total number of textures the material has
  if (newMat.pBaseColor) {
    newMat.pLinearTextures.emplace_back(newMat.pBaseColor);
  }
  if (newMat.pNormal) {
    newMat.pLinearTextures.emplace_back(newMat.pNormal);
  }
  if (newMat.pMetalRoughness) {
    newMat.pLinearTextures.emplace_back(newMat.pMetalRoughness);
  }
  if (newMat.pOcclusion) {
    newMat.pLinearTextures.emplace_back(newMat.pOcclusion);
  }
  if (newMat.pEmissive) {
    newMat.pLinearTextures.emplace_back(newMat.pEmissive);
  }
  if (newMat.pExtra0) {
    newMat.pLinearTextures.emplace_back(newMat.pExtra0);
  }
  if (newMat.pExtra1) {
    newMat.pLinearTextures.emplace_back(newMat.pExtra1);
  }
  if (newMat.pExtra2) {
    newMat.pLinearTextures.emplace_back(newMat.pExtra2);
  }

  newMat.passFlags = pDesc->passFlags;

  // determine pipeline automatically if not explicitly set
  if (pDesc->passFlags == EDynamicRenderingPass::Null) {
    switch (pDesc->alphaMode) {
      case EAlphaMode::Blend: {
        newMat.passFlags |= EDynamicRenderingPass::BlendCullNone;
        break;
      }
      case EAlphaMode::Mask: {
        newMat.passFlags |= EDynamicRenderingPass::MaskCullBack;
        break;
      }
      case EAlphaMode::Opaque: {
        newMat.passFlags |= pDesc->doubleSided ? EDynamicRenderingPass::OpaqueCullNone
                                               : EDynamicRenderingPass::OpaqueCullBack;
        break;
      }
    }

    // Check material for a single bit alpha and reassign it to OpaqueCullNone pass
    if ((newMat.passFlags & EDynamicRenderingPass::BlendCullNone)
      && (newMat.pBaseColor->texture.imageFormat == VK_FORMAT_BC1_RGBA_SRGB_BLOCK
        || newMat.pBaseColor->texture.imageFormat == VK_FORMAT_BC1_RGBA_UNORM_BLOCK)) {
      newMat.passFlags ^= EDynamicRenderingPass::BlendCullNone;
      newMat.passFlags |= EDynamicRenderingPass::DiscardCullNone;
    }

    // All glTF materials are featured in shadow prepass by default, discard materials have their dedicated shadow pass
    newMat.passFlags |= (newMat.passFlags & EDynamicRenderingPass::DiscardCullNone)
      ? EDynamicRenderingPass::ShadowDiscard : EDynamicRenderingPass::Shadow;
  }

  RE_LOG(Log, "Creating material \"%s\".", newMat.name.c_str());
  m_materials.at(pDesc->name) = std::make_unique<RMaterial>(std::move(newMat));

  RMaterial* pNewMaterial = m_materials.at(pDesc->name).get();

  // Store material's data block in the buffer at an appropriate index
  pNewMaterial->bufferIndex = getMaterialBufferIndex(pNewMaterial);

  if (pNewMaterial->bufferIndex == -1) {
    RE_LOG(Error, "Material buffer is out of free space.");
  } else {
    RSceneFragmentPCB* pMemAddress = (RSceneFragmentPCB*)core::renderer.getMaterialData()->buffer.allocInfo.pMappedData
      + pNewMaterial->bufferIndex;
    memcpy(pMemAddress, &pNewMaterial->pushConstantBlock, sizeof(RSceneFragmentPCB));
  }

  return pNewMaterial;
}

RMaterial* core::MResources::getMaterial(const char* name) noexcept {
  if (m_materials.contains(name)) {
    return m_materials.at(name).get();
  }

#ifndef NDEBUG
  RE_LOG(Error, "Material '%s' not found.", name);
#endif

  // returns default material if present
  return (m_materials.size() > 0) ? m_materials[0].get() : nullptr;
}

uint32_t core::MResources::getMaterialCount() const noexcept {
  return static_cast<uint32_t>(m_materials.size());
}

uint32_t core::MResources::getMaterialBufferIndex(RMaterial* pMaterial) noexcept {
  if (pMaterial->bufferIndex != -1) {
    if (m_materialIndices[pMaterial->bufferIndex] != pMaterial) {
      RE_LOG(Error, "Invalid material buffer index %d for '%s'. Possible data corruption.",
        pMaterial->bufferIndex, pMaterial->name.c_str());
    }

    return pMaterial->bufferIndex;
  }

  uint32_t index = 0;
  for (auto& it : m_materialIndices) {
    if (it == nullptr) {
      it = pMaterial;
      return index;
    }

    ++index;
  }

  return -1;
}

TResult core::MResources::deleteMaterial(const char* name) noexcept {
  if (m_materials.contains(name)) {
    m_materials.at(name).reset();
    m_materials.erase(name);

    return RE_OK;
  }

  RE_LOG(Warning, "Could not delete material \"%\", does not exist.", name);
  return RE_WARNING;
}

std::vector<RTexture*>* core::MResources::getMaterialTextures(
    const char* name) noexcept {
  RMaterial* pMaterial = getMaterial(name);

  if (!pMaterial) {
    RE_LOG(Error, "Couldn't retrieve textures from requested material '%s'.",
           name);
    return nullptr;
  }

  return &pMaterial->pLinearTextures;
}
