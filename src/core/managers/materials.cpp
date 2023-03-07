#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/materials.h"

core::MMaterials::MMaterials() {
  RE_LOG(Log, "Setting up the materials manager.");
}

void core::MMaterials::initialize() {
  RE_LOG(Log, "Initializing materials manager data.");

  // create null texture
  RSamplerInfo samplerInfo{};
  if (loadTexture(RE_PATH_NULLTEXTURE, &samplerInfo) != RE_OK) {
    RE_LOG(Error, "Failed to load null texture.");
  }
}

core::MMaterials::RMaterial* core::MMaterials::createMaterial(
    RMaterialInfo* pDesc) noexcept {

  if (!m_materials.try_emplace(pDesc->name).second) {
    RE_LOG(Warning, "Material with the name \"%s\" is already created.");
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
  newMat.pushConstantBlock.alphaMode = static_cast<float>(pDesc->alphaMode);
  newMat.pushConstantBlock.alphaCutoff = pDesc->alphaCutoff;

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

  // create descriptor set for Vulkan API

  /* OBSOLETE
  newMat.data.x = pDesc->materialIntensity;
  newMat.data.y = pDesc->metalnessFactor;
  newMat.data.z = pDesc->roughnessFactor;

  // bump coefficient for shader needs to be reversed, so 2 times bump intensity
  // will equal 0.5 in the shader
  float bumpCoef = 1.0f - ((pDesc->bumpIntensity - 1.0f) * 0.5f);
  newMat.data.w =
      std::max(0.05f, bumpCoef);  // safeguard against zero or negative values

  newMat.F0 = pDesc->F0;
  newMat.manageTextures = pDesc->manageTextures;
  newMat.effectFlags = pDesc->effectFlags;
  // OBSOLETE ^ */

  RE_LOG(Log, "Creating material \"%s\".", newMat.name.c_str());
  m_materials.at(pDesc->name) = std::make_unique<RMaterial>(std::move(newMat));
  m_materials.at(pDesc->name)->createDescriptorSet();
  return m_materials.at(pDesc->name).get();
}

core::MMaterials::RMaterial* core::MMaterials::getMaterial(const char* name) noexcept {
  if (m_materials.contains(name)) {
    return m_materials.at(name).get();
  }

#ifndef NDEBUG
  RE_LOG(Error, "Material '%s' not found.", name);
#endif

  // returns default material if present
  return (m_materials.size() > 0) ? m_materials[0].get() : nullptr;
}

uint32_t core::MMaterials::getMaterialCount() const noexcept {
  return static_cast<uint32_t>(m_materials.size());
}

TResult core::MMaterials::deleteMaterial(const char* name) noexcept {
  if (m_materials.contains(name)) {
    m_materials.at(name).reset();
    m_materials.erase(name);

    return RE_OK;
  }

  RE_LOG(Warning, "Could not delete material \"%\", does not exist.", name);
  return RE_WARNING;
}

void core::MMaterials::RMaterial::createDescriptorSet() {
  // allocate material's descriptor set
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = core::renderer.getDescriptorPool();
  allocateInfo.pSetLayouts =
      &core::renderer.getDescriptorSetLayouts()->material;
  allocateInfo.descriptorSetCount = 1;

  if (vkAllocateDescriptorSets(core::renderer.logicalDevice.device,
                               &allocateInfo, &descriptorSet) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to allocate descriptor set for the material \"%\".",
           name);
    return;
  };

  RTexture* pNullTexture = core::materials.getTexture(RE_PATH_NULLTEXTURE);

  if (!pNullTexture) {
    RE_LOG(Error,
           "Missing data. Materials manager was not initialized correctly.");
    return;
  }

  // retrieve all material's texture image descriptors
  std::vector<VkDescriptorImageInfo> imageDescriptors = {
      pBaseColor ? pBaseColor->texture.descriptor
                 : pNullTexture->texture.descriptor,
      pNormal ? pNormal->texture.descriptor : pNullTexture->texture.descriptor,
      pMetalRoughness ? pMetalRoughness->texture.descriptor
                      : pNullTexture->texture.descriptor,
      pOcclusion ? pOcclusion->texture.descriptor
                 : pNullTexture->texture.descriptor,
      pEmissive ? pEmissive->texture.descriptor
                : pNullTexture->texture.descriptor,
      pExtra ? pExtra->texture.descriptor : pNullTexture->texture.descriptor,
  };

  // write retrieved data to newly allocated descriptor set
  std::vector<VkWriteDescriptorSet> writeDescriptorSets;
  uint32_t writeSize = static_cast<uint32_t>(imageDescriptors.size());
  writeDescriptorSets.resize(writeSize);

  for (uint32_t i = 0; i < writeSize; ++i) {
    writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[i].dstSet = descriptorSet;
    writeDescriptorSets[i].dstBinding = i;
    writeDescriptorSets[i].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
  }

  vkUpdateDescriptorSets(core::renderer.logicalDevice.device, writeSize,
                         writeDescriptorSets.data(), 0, nullptr);
}