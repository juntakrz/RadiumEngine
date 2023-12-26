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

  // create G-buffer material physical render target will contain metalness,
  // roughness and ambient occlusion so occlusion texture will be used as a
  // source for fragment position instead
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
  materialInfo.pipelineFlags = EPipeline::PBRDeferred;

  if (!createMaterial(&materialInfo)) {
    RE_LOG(Critical, "Failed to create G-Buffer material.");

    return;
  }

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

  newMat.pushConstantBlock.bumpIntensity = pDesc->bumpIntensity;
  newMat.pushConstantBlock.materialIntensity = pDesc->materialIntensity;
  newMat.pushConstantBlock.f0 = pDesc->F0;

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
