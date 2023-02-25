#include "pch.h"
#include "core/managers/materials.h"

core::MMaterials::MMaterials() { RE_LOG(Log, "Setting up the materials manager."); }

uint32_t core::MMaterials::addMaterial(DMaterial* pDesc) noexcept {
  uint16_t index = 0;

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

  /*
  pDesc->textures.tex0 != ""
      ? newMat.idTex[0] = addTexture(pDesc->textures.tex0)
      : newMat.idTex[0] = addTexture("default//default.dds");

  pDesc->textures.tex1 != ""
      ? newMat.idTex[1] = addTexture(pDesc->textures.tex1)
      : newMat.idTex[1] = addTexture("default//default_n.dds");

  pDesc->textures.tex2 != ""
      ? newMat.idTex[2] = addTexture(pDesc->textures.tex2)
      : newMat.idTex[2] = addTexture("default//default_metallic.dds");

  pDesc->textures.tex3 != ""
      ? newMat.idTex[3] = addTexture(pDesc->textures.tex3)
      : newMat.idTex[3] = addTexture("default//default_s.dds");

  pDesc->textures.tex4 != ""
      ? newMat.idTex[4] = addTexture(pDesc->textures.tex4)
      : newMat.idTex[4] = addTexture("default//default_s.dds");

  pDesc->textures.tex5 != ""
      ? newMat.idTex[5] = addTexture(pDesc->textures.tex5)
      : newMat.idTex[5] = addTexture("default//default_s.dds");
  */

  newMat.ambientColor = pDesc->material.ambientColor;
  newMat.data.x = pDesc->material.materialIntensity;
  newMat.data.y = pDesc->material.specular_metalness;
  newMat.data.z = pDesc->material.pow_roughness;

  // bump coefficient for shader needs to be reversed, so 2 times bump intensity
  // will equal 0.5 in the shader
  float bumpCoef = 1.0f - ((pDesc->material.bumpIntensity - 1.0f) * 0.5f);
  newMat.data.w =
      std::max(0.05f, bumpCoef);  // safeguard against zero or negative values

  newMat.F0 = pDesc->material.F0;
  newMat.manageTextures = pDesc->manageTextures;
  newMat.effects = pDesc->effects;

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
        for (uint32_t i = 0; i < sizeof(it->idTex) / sizeof(uint32_t); i++) {
          //(it->idTex[i] != "") ? deleteTexture(it->idTex[i]) : 0;
        }
      }
      m_materials.erase(m_materials.begin() + it_n);
      return;
    }
    it_n++;
  }
}
