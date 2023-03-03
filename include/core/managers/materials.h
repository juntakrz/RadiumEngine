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
  struct RMaterial {
    uint32_t id;
    std::string name;

    std::string shaderVertex, shaderPixel, shaderGeometry;
    std::string idTex[6] = {"", "", "", "", "", ""};

    glm::vec4 ambientColor;
    glm::vec4 F0;

    /* Material Data Structure
    *
    x = material intensity
    y = specular intensity	/ metalness (PBS)
    z = specular power		/ roughness (PBS)
    w = bumpmapping intensity
    */
    glm::vec4 data;

    // add effects using bitwise ops
    uint32_t effects;

    // will materials manager automatically try to delete textures
    // from memory if unused by any other material
    bool manageTextures = false;
  };

  struct RTexture {
    std::string name = "";
    std::string filePath = "";
    ktxVulkanTexture texture;

    ~RTexture();
  };

  /*
  struct DFShader {
    COMPTR<ID3D11VertexShader> pVS = nullptr;
    COMPTR<ID3D11GeometryShader> pGS = nullptr;
    COMPTR<ID3D11PixelShader> pPS = nullptr;
    COMPTR<ID3DBlob> pSData;
  };*/

  std::vector<std::unique_ptr<RMaterial>> m_materials;
  std::unordered_map<std::string, RTexture> m_textures;
  // std::unordered_map<std::string, DFShader> m_Shaders;

 public:
  struct RMaterialDescriptor {
    std::string name;
    bool manageTextures = false;

    struct {
      std::string vertex = "default.vert", pixel = "default.frag",
                  geometry = "";
    } shaders;

    struct {
      std::string tex0 = "default//default.dds", tex1 = "", tex2 = "",
                  tex3 = "", tex4 = "", tex5 = "";
    } textures;

    struct {
      glm::vec4 ambientColor = {0.0f, 0.0f, 0.0f, 0.0f};
      glm::vec4 F0 = {0.4f, 0.4f, 0.4f, 0.0f};  // basic metal
      float materialIntensity = 1.0f;
      float metalness = 1.0f;
      float roughness = 1.0f;
      float bumpIntensity = 1.0f;
    } material;

    uint32_t effects = 0;
  };

 private:
  MMaterials();

 public:
  static MMaterials &get() {
    static MMaterials _sInstance;
    return _sInstance;
  }

  // MATERIALS

  // add new material, returns material id
  uint32_t addMaterial(RMaterialDescriptor *pDesc) noexcept;

  RMaterial *getMaterial(std::string name) noexcept;
  RMaterial *getMaterial(uint32_t index) noexcept;
  uint32_t getMaterialIndex(std::string name) const noexcept;
  uint32_t getMaterialCount() const noexcept;
  void deleteMaterial(uint32_t index) noexcept;
  void deleteMaterial(std::string name) noexcept;

  // TEXTURES

  // load texture from DDS or KTX file
  void loadTexture(const std::string& filePath);

  void destroyAllTextures();
};
}  // namespace core