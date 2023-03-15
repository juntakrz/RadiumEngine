#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/resources.h"

core::MResources::MResources() {
  RE_LOG(Log, "Setting up the materials manager.");
}

void core::MResources::initialize() {
  RE_LOG(Log, "Initializing materials manager data.");

  // create null texture
  RSamplerInfo samplerInfo{};
  if (loadTexture(RE_DEFAULTTEXTURE, &samplerInfo) != RE_OK) {
    RE_LOG(Error, "Failed to load default texture.");
  }

  if (loadTexture(RE_BLACKTEXTURE, &samplerInfo) != RE_OK) {
    RE_LOG(Error, "Failed to load default black texture.");
  }

  if (loadTexture(RE_WHITETEXTURE, &samplerInfo) != RE_OK) {
    RE_LOG(Error, "Failed to load default white texture.");
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
  newMat.alphaMode = pDesc->alphaMode;
  newMat.doubleSided = pDesc->doubleSided;

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