#pragma once

#include "common.h"
#include "core/objects.h"

class WPrimitive;

namespace tinygltf {
class Model;
class Node;
}  // namespace tinygltf

namespace core {

class MMaterials {
  friend class ::WPrimitive;

 private:

  // if texture is loaded using KTX library - VMA allocations are not used
  struct RTexture {
    std::string name = "";
    RVulkanTexture texture;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    bool isKTX = false;

    // trying to track how many times texture is assigned to material
    // to see if it should be deleted if owning material is deleted
    uint32_t references = 0;

    TResult createImageView();
    TResult createSampler(const RSamplerInfo *pSamplerInfo);
    TResult createDescriptor();
    ~RTexture();
  };

  struct RMaterial {
    std::string name;
    bool doubleSided = false;
    EAlphaMode alphaMode;

    std::string shaderVertex, shaderPixel, shaderGeometry;
    RTexture* pBaseColor        = nullptr;
    RTexture* pNormal           = nullptr;
    RTexture* pMetalRoughness   = nullptr;
    RTexture* pOcclusion        = nullptr;
    RTexture* pEmissive         = nullptr;
    RTexture* pExtra            = nullptr;

    RPushConstantBlock_Material pushConstantBlock;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    // OBSOLETE
    glm::vec4 F0;

    // add effects using bitwise ops
    uint32_t effectFlags;

    // will materials manager automatically try to delete textures
    // from memory if unused by any other material
    // !requires sharedPtr code, currently unused!
    bool manageTextures = false;

    void createDescriptorSet();
  };

  std::unordered_map<std::string, std::unique_ptr<RMaterial>> m_materials;
  std::unordered_map<std::string, RTexture> m_textures;

 private:
  MMaterials();

 public:
  static MMaterials &get() {
    static MMaterials _sInstance;
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
  TResult loadTexture(const std::string &filePath,
                      const RSamplerInfo* pSamplerInfo);

  // create new texture with specific parameters
  void createTexture(const char* name, uint32_t width, uint32_t height,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, RTexture* pTexture,
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