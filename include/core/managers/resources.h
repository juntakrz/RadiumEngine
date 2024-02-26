#pragma once

#include "common.h"
#include "core/objects.h"
#include "core/material/material.h"
#include "core/material/texture.h"

class WPrimitive;

namespace tinygltf {
class Model;
class Node;
}  // namespace tinygltf

namespace core {

class MResources {
  friend class ::WPrimitive;

 private:
  std::unordered_map<std::string, std::unique_ptr<RMaterial>> m_materials;
  std::unordered_map<std::string, RTexture> m_textures;

  std::vector<RTexture*> m_samplerIndices;

 private:
  MResources();

 public:
  static MResources &get() {
    static MResources _sInstance;
    return _sInstance;
  }

  void initialize();
  uint32_t getFreeCombinedSamplerIndex();
  void updateMaterialDescriptorSet(RTexture* pTexture, EResourceType resourceType);

  // MATERIALS

  // create new material, returns pointer to the new material
  RMaterial* createMaterial(RMaterialInfo *pDesc) noexcept;

  RMaterial* getMaterial(const char* name) noexcept;
  uint32_t getMaterialCount() const noexcept;
  TResult deleteMaterial(const char* name) noexcept;

  std::vector<RTexture*>* getMaterialTextures(const char* name) noexcept;

  // TEXTURES

  // load texture from KTX file (versions 1 and 2 are supported)
  // won't create sampler if no sampler info is provided
  TResult loadTexture(const std::string& filePath, RSamplerInfo* pSamplerInfo,
                      const bool createExtraViews = false);

  // load KTX texture to staging buffer only
  TResult loadTextureToBuffer(const std::string& filePath, RBuffer& outBuffer);

  // load PNG image, won't create sampler if no sampler info is provided
  TResult loadTexturePNG(const std::string& filePath,
                         RSamplerInfo* pSamplerInfo);

  // create new texture with specific parameters
  RTexture* createTexture(RTextureInfo* pInfo);

  // create new uninitialized texture entry
  RTexture* createTexture(const std::string& name);

  // write to texture, must have proper layout
  TResult writeTexture(RTexture* pTexture, void* pData, VkDeviceSize dataSize);

  // unconditional destruction of texture object if 'force' is true
  bool destroyTexture(const char* name, bool force = false) noexcept;

  // should be used only for changing settings within texture object
  // for assigning texture to material use assignTexture() instead
  RTexture* getTexture(const char* name) noexcept;

  // retrieve texture to be assigned in material instance
  RTexture* const assignTexture(const char* name) noexcept;

  void destroyAllTextures();
};
}  // namespace core