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

 private:
  MResources();

 public:
  static MResources &get() {
    static MResources _sInstance;
    return _sInstance;
  }

  void initialize();

  // MATERIALS

  // create new material, returns pointer to the new material
  RMaterial* createMaterial(RMaterialInfo *pDesc) noexcept;

  RMaterial* getMaterial(const char* name) noexcept;
  uint32_t getMaterialCount() const noexcept;
  TResult deleteMaterial(const char* name) noexcept;

  // TEXTURES

  // load texture from KTX file (versions 1 and 2 are supported)
  // won't create sampler if no sampler info is provided
  TResult loadTexture(const std::string& filePath,
                      RSamplerInfo* pSamplerInfo);

  // load KTX texture to staging buffer only
  TResult loadTextureToBuffer(const std::string& filePath, RBuffer& outBuffer);

  // load PNG image, won't create sampler if no sampler info is provided
  TResult loadTexturePNG(const std::string& filePath,
                         RSamplerInfo* pSamplerInfo);

  // create new texture with specific parameters
  TResult createTexture(const char* name, uint32_t width, uint32_t height,
                        VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage, bool asCubemap = false,
                        void* pData = nullptr, VkDeviceSize inDataSize = 0u,
                        VkMemoryPropertyFlags* properties = nullptr);

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