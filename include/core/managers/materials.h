#pragma once

#include "common.h"
#include "core/objects.h"

namespace tinygltf {
class Model;
class Node;
}  // namespace tinygltf

namespace core {

class MMaterials {
 private:

  // if texture is loaded using KTX library - VMA allocations are not used
  struct RTexture {
    std::string name = "";
    RVulkanTexture texture;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    bool isKTX = false;

    TResult createImageView();
    TResult createSampler(const RSamplerInfo *pSamplerInfo);
    TResult createDescriptor();
    ~RTexture();
  };

  struct RMaterial {
    uint32_t id;
    std::string name;
    bool doubleSided = false;

    std::string shaderVertex, shaderPixel, shaderGeometry;
    RTexture* pBaseColor        = nullptr;
    RTexture* pNormal           = nullptr;
    RTexture* pMetalRoughness   = nullptr;
    RTexture* pOcclusion        = nullptr;
    RTexture* pEmissive         = nullptr;
    RTexture* pExtra            = nullptr;

    RPushConstantBlock_Material pushConstantBlock;
    VkDescriptorSet descriptorSet;
    
    // OBSOLETE
    glm::vec4 F0;

    // add effects using bitwise ops
    uint32_t effectFlags;

    // will materials manager automatically try to delete textures
    // from memory if unused by any other material
    // !requires sharedPtr code, currently unused!
    bool manageTextures = false;
  };

  std::vector<std::unique_ptr<RMaterial>> m_materials;
  std::unordered_map<std::string, RTexture> m_textures;

 private:
  MMaterials();

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);

 public:
  static MMaterials &get() {
    static MMaterials _sInstance;
    return _sInstance;
  }

  // MATERIALS

  // create new material, returns material's index
  uint32_t createMaterial(RMaterialInfo *pDesc) noexcept;

  RMaterial* getMaterial(std::string name) noexcept;
  RMaterial* getMaterial(uint32_t index) noexcept;
  uint32_t getMaterialIndex(std::string name) const noexcept;
  uint32_t getMaterialCount() const noexcept;
  void deleteMaterial(uint32_t index) noexcept;
  void deleteMaterial(std::string name) noexcept;

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

  RTexture* getTexture(const char* name) noexcept;

  void destroyAllTextures();
};
}  // namespace core