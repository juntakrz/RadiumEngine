#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/resources.h"

core::MResources::MResources() {
  RE_LOG(Log, "Setting up the materials manager.");
}

void core::MResources::initialize() {
  RE_LOG(Log, "Initializing materials manager data.");

  m_sampler2DIndices.resize(config::scene::sampler2DBudget, nullptr);

  // create the "default" material
  RSamplerInfo samplerInfo{};

  loadTexture(RE_DEFAULTTEXTURE, &samplerInfo);
  loadTexture("default/default_normal.ktx2", &samplerInfo);
  loadTexture("default/default_metallicRoughness.ktx2", &samplerInfo);
  loadTexture("default/default_occlusion.ktx2", &samplerInfo);
  loadTexture(RE_BLACKTEXTURE, &samplerInfo);
  loadTexture(RE_WHITETEXTURE, &samplerInfo);

  // create default material
  RMaterialInfo materialInfo{};
  materialInfo.name = "default";
  materialInfo.pipelineFlags = EPipeline::OpaqueCullBack;
  materialInfo.textures.baseColor = RE_DEFAULTTEXTURE;
  materialInfo.textures.normal = "default/default_normal.ktx2";
  materialInfo.textures.metalRoughness =
      "default/default_metallicRoughness.ktx2";
  materialInfo.textures.occlusion = "default/default_occlusion.ktx2";
  materialInfo.texCoordSets.baseColor = 0;
  materialInfo.texCoordSets.normal = 0;
  materialInfo.texCoordSets.metalRoughness = 0;
  materialInfo.texCoordSets.occlusion = 0;

  createMaterial(&materialInfo);

  // create present material that takes combined output of all render passes as
  // a shader read only attachment
  materialInfo = RMaterialInfo{};
  materialInfo.name = RMAT_PRESENT;
  materialInfo.textures.baseColor = RTGT_GPBR;
  materialInfo.alphaMode = EAlphaMode::Opaque;
  materialInfo.doubleSided = false;
  materialInfo.manageTextures = true;
  materialInfo.pipelineFlags = EPipeline::Present;

  if (!createMaterial(&materialInfo)) {
    RE_LOG(Critical, "Failed to create Vulkan present material.");

    return;
  }
}

uint32_t core::MResources::getFreeSampler2DIndex() {
  for (uint32_t i = 0; i < m_sampler2DIndices.size(); ++i) {
    if (m_sampler2DIndices[i] == nullptr) return i;
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
    uint32_t index = getFreeSampler2DIndex();

    VkDescriptorImageInfo imageInfo = pTexture->texture.descriptor;

    // HACK: this image is expected to be translated to a proper layout later
    if (imageInfo.imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSet.dstBinding = 0;
    writeSet.dstArrayElement = index;
    writeSet.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(core::renderer.logicalDevice.device, 1u, &writeSet, 0, nullptr);

    // Store index data
    pTexture->sampler2DIndex = index;
    m_sampler2DIndices[index] = pTexture;

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

  RMaterial newMat;
  newMat.name = pDesc->name;
  newMat.shaderPixel = pDesc->shaders.pixel;
  newMat.shaderGeometry = pDesc->shaders.geometry;
  newMat.shaderVertex = pDesc->shaders.vertex;

  newMat.pBaseColor = assignTexture(pDesc->textures.baseColor.c_str());
  newMat.pNormal = assignTexture(pDesc->textures.normal.c_str());
  newMat.pMetalRoughness = assignTexture(pDesc->textures.metalRoughness.c_str());
  newMat.pOcclusion = assignTexture(pDesc->textures.occlusion.c_str());
  newMat.pEmissive = assignTexture(pDesc->textures.emissive.c_str());
  newMat.pExtra = assignTexture(pDesc->textures.extra.c_str());

  newMat.pushConstantBlock.metallicFactor = pDesc->metallicFactor;
  newMat.pushConstantBlock.roughnessFactor = pDesc->roughnessFactor;
  newMat.pushConstantBlock.alphaMode = static_cast<float>(pDesc->alphaMode);
  newMat.pushConstantBlock.alphaCutoff = pDesc->alphaCutoff;
  newMat.pushConstantBlock.bumpIntensity = pDesc->bumpIntensity;
  newMat.pushConstantBlock.emissiveIntensity = pDesc->emissiveIntensity;

  // disable reading from texture in shader if no texture is available
  newMat.pushConstantBlock.baseColorTextureSet = newMat.pBaseColor ? pDesc->texCoordSets.baseColor : -1;
  newMat.pushConstantBlock.normalTextureSet = newMat.pNormal ? pDesc->texCoordSets.normal : -1;
  newMat.pushConstantBlock.metallicRoughnessTextureSet = newMat.pMetalRoughness ? pDesc->texCoordSets.metalRoughness : -1;
  newMat.pushConstantBlock.occlusionTextureSet = newMat.pOcclusion ? pDesc->texCoordSets.occlusion : -1;
  newMat.pushConstantBlock.emissiveTextureSet = newMat.pEmissive ? pDesc->texCoordSets.emissive : -1;
  newMat.pushConstantBlock.extraTextureSet = newMat.pExtra ? pDesc->texCoordSets.extra : -1;

  newMat.pushConstantBlock.samplerIndex[0] = newMat.pBaseColor ? newMat.pBaseColor->sampler2DIndex : 0;
  newMat.pushConstantBlock.samplerIndex[1] = newMat.pNormal ? newMat.pNormal->sampler2DIndex : 0;
  newMat.pushConstantBlock.samplerIndex[2] = newMat.pMetalRoughness ? newMat.pMetalRoughness->sampler2DIndex : 0;
  newMat.pushConstantBlock.samplerIndex[3] = newMat.pOcclusion ? newMat.pOcclusion->sampler2DIndex : 0;
  newMat.pushConstantBlock.samplerIndex[4] = newMat.pEmissive ? newMat.pEmissive->sampler2DIndex : 0;
  newMat.pushConstantBlock.samplerIndex[5] = newMat.pExtra ? newMat.pExtra->sampler2DIndex : 0;

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
  if (newMat.pExtra) {
    newMat.pLinearTextures.emplace_back(newMat.pExtra);
  }

  newMat.pipelineFlags = pDesc->pipelineFlags;

  // determine pipeline automatically if not explicitly set
  if (pDesc->pipelineFlags == EPipeline::Null) {
    switch (pDesc->alphaMode) {
      case EAlphaMode::Blend: {
        newMat.pipelineFlags |= EPipeline::BlendCullNone;
        break;
      }
      case EAlphaMode::Mask: {
        newMat.pipelineFlags |= EPipeline::MaskCullBack;
        break;
      }
      case EAlphaMode::Opaque: {
        newMat.pipelineFlags |= pDesc->doubleSided ? EPipeline::OpaqueCullNone
                                                   : EPipeline::OpaqueCullBack;
        break;
      }
    }

    // all glTF materials are featured in shadow prepass by default
    newMat.pipelineFlags |= EPipeline::Shadow;
  }

  RE_LOG(Log, "Creating material \"%s\".", newMat.name.c_str());
  m_materials.at(pDesc->name) = std::make_unique<RMaterial>(std::move(newMat));
  m_materials.at(pDesc->name)->createDescriptorSet();
  return m_materials.at(pDesc->name).get();
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
