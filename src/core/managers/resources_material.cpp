#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/resources.h"

core::MResources::MResources() {
  RE_LOG(Log, "Setting up the materials manager.");
}

void core::MResources::initialize() {
  RE_LOG(Log, "Initializing materials manager data.");

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

  newMat.pushConstantBlock.baseColorFactor = pDesc->baseColorFactor;
  newMat.pushConstantBlock.emissiveFactor = pDesc->emissiveFactor;
  newMat.pushConstantBlock.metallicFactor = pDesc->metallicFactor;
  newMat.pushConstantBlock.roughnessFactor = pDesc->roughnessFactor;
  newMat.pushConstantBlock.alphaMode = static_cast<float>(pDesc->alphaMode);
  newMat.pushConstantBlock.alphaCutoff = pDesc->alphaCutoff;

  // disable reading from texture in shader if no texture is available
  newMat.pushConstantBlock.baseColorTextureSet =
      newMat.pBaseColor ? pDesc->texCoordSets.baseColor : -1;
  newMat.pushConstantBlock.normalTextureSet =
      newMat.pNormal ? pDesc->texCoordSets.normal : -1;
  newMat.pushConstantBlock.metallicRoughnessTextureSet =
      newMat.pMetalRoughness ? pDesc->texCoordSets.metalRoughness : -1;
  newMat.pushConstantBlock.occlusionTextureSet =
      newMat.pOcclusion ? pDesc->texCoordSets.occlusion : -1;
  newMat.pushConstantBlock.emissiveTextureSet =
      newMat.pEmissive ? pDesc->texCoordSets.emissive : -1;
  newMat.pushConstantBlock.extraTextureSet =
      newMat.pExtra ? pDesc->texCoordSets.extra : -1;

  newMat.pushConstantBlock.bumpIntensity = pDesc->bumpIntensity;
  newMat.pushConstantBlock.materialIntensity = pDesc->materialIntensity;
  newMat.pushConstantBlock.f0 = pDesc->F0;

  newMat.pipelineFlags = pDesc->pipelineFlags;

  // determine pipeline automatically if not explicitly set
  if (pDesc->pipelineFlags == EPipeline::Null) {
    switch (pDesc->alphaMode) {
      case EAlphaMode::Blend: {
        newMat.pipelineFlags |= EPipeline::BlendCullBack;
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

    // all glTF materials are featured in depth prepass by default
    newMat.pipelineFlags |= EPipeline::Depth;
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