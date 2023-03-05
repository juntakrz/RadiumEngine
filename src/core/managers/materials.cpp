#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/materials.h"

core::MMaterials::MMaterials() {
  RE_LOG(Log, "Setting up the materials manager.");
}

uint32_t core::MMaterials::createMaterial(RMaterialInfo* pDesc) noexcept {
  // find current free material index
  uint32_t index = 0;

  for (const auto& it : m_materials) {
    if (it->name == pDesc->name) {
      return index;
    }

    if (index < it->id) {
      break;
    }
    index++;
  }

  RMaterial newMat;
  newMat.id = index;
  newMat.name = pDesc->name;
  newMat.shaderPixel = pDesc->shaders.pixel;
  newMat.shaderGeometry = pDesc->shaders.geometry;
  newMat.shaderVertex = pDesc->shaders.vertex;

  newMat.pBaseColor = getTexture(pDesc->textures.baseColor.c_str());
  newMat.pNormal = getTexture(pDesc->textures.normal.c_str());
  newMat.pMetalRoughness = getTexture(pDesc->textures.metalRoughness.c_str());
  newMat.pOcclusion = getTexture(pDesc->textures.occlusion.c_str());
  newMat.pEmissive = getTexture(pDesc->textures.emissive.c_str());
  newMat.pExtra = getTexture(pDesc->textures.extra.c_str());

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

  m_materials.emplace_back(
      std::make_unique<RMaterial>(std::move(newMat)));
  return index;
}

core::MMaterials::RMaterial* core::MMaterials::getMaterial(std::string name) noexcept {
  for (const auto& it : m_materials) {
    if (name == it->name) {
      return it.get();
    }
  }

#ifndef NDEBUG
  RE_LOG(Error, "Material '%s' not found.", name.c_str());
#endif

  return (m_materials.size() > 0) ? m_materials[0].get() : nullptr;
}

core::MMaterials::RMaterial* core::MMaterials::getMaterial(
    uint32_t index) noexcept {
  for (const auto& it : m_materials) {
    if (it->id == index) {
      return m_materials[index].get();
    }
  }

#ifndef NDEBUG
  RE_LOG(Error, "Material index out of bounds.");
#endif

  return (m_materials.size() > 0) ? m_materials[0].get() : nullptr;
}

uint32_t core::MMaterials::getMaterialIndex(std::string name) const noexcept {
  uint32_t index = 0;
  for (const auto& it : m_materials) {
    if (name == it->name) {
      return index;
    }
    index++;
  }

#ifndef NDEBUG
  RE_LOG(Error, "Index for material '%s' not found.", name);
#endif
  return 0;
}

uint32_t core::MMaterials::getMaterialCount() const noexcept {
  return static_cast<uint32_t>(m_materials.size());
}

void core::MMaterials::deleteMaterial(uint32_t index) noexcept {
  uint32_t it_n = 0;

  for (auto& it : m_materials) {
    if (it->id == index && index > 0) {
      if (it->manageTextures) {
        for (uint32_t i = 0; i < RE_MAXTEXTURES; i++) {
          //(it->idTex[i] != "") ? deleteTexture(it->idTex[i]) : 0;
        }
      }
      m_materials.erase(m_materials.begin() + it_n);
      return;
    }
    it_n++;
  }
}

void core::MMaterials::deleteMaterial(std::string name) noexcept {
  uint32_t it_n = 0;

  for (auto& it : m_materials) {
    if (it->name == name && it_n > 0) {
      if (it->manageTextures) {
        //for (uint32_t i = 0; i < sizeof(it->idTex) / sizeof(uint32_t); i++) {
          //(it->idTex[i] != "") ? deleteTexture(it->idTex[i]) : 0;
        //}
      }
      m_materials.erase(m_materials.begin() + it_n);
      return;
    }
    it_n++;
  }
}