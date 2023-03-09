#pragma once

#include "vk_mem_alloc.h"
#include "config.h"

enum class EBufferMode {  // VkBuffer creation mode
  CPU_UNIFORM,        // create uniform buffer for GPU programs
  CPU_VERTEX,         // create vertex buffer for the iGPU (UNUSED)
  CPU_INDEX,          // create index buffer for the iGPU (UNUSED)
  DGPU_VERTEX,        // create dedicated GPU vertex buffer
  DGPU_INDEX,         // create dedicated GPU index buffer
  STAGING             // create staging buffer only
};

enum class ECmdType {
  Graphics,
  Compute,
  Transfer,
  Present
};

enum class EActorType {  // actor type
  Base,
  Camera,
  Pawn
};

enum class EWPrimitive {
  Null,
  Plane,
  Sphere,
  Cube
};

enum class ERAlphaMode {
  Blend,
  Mask,
  Opaque
};

struct RVkQueueFamilyIndices {
  std::vector<int32_t> graphics;
  std::vector<int32_t> compute;
  std::vector<int32_t> present;
  std::vector<int32_t> transfer;

  // retrieve first entries for queue family indices
  std::set<int32_t> getAsSet() const;
};

struct RVkSwapChainInfo {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> modes;
};

struct RVkPhysicalDevice {
  VkPhysicalDevice device = VK_NULL_HANDLE;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceMemoryProperties memProperties;
  RVkQueueFamilyIndices queueFamilyIndices;
  RVkSwapChainInfo swapChainInfo;
  bool bIsValid = false;
};

// used by renderer
struct RDescriptorSetLayouts {
  VkDescriptorSetLayout scene;
  VkDescriptorSetLayout material;
  VkDescriptorSetLayout node;
};

struct RVkLogicalDevice {
  VkDevice device;

  struct {
    VkQueue graphics  = VK_NULL_HANDLE;
    VkQueue compute   = VK_NULL_HANDLE;
    VkQueue present   = VK_NULL_HANDLE;
    VkQueue transfer  = VK_NULL_HANDLE;
  } queues;
};

struct RBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
};

struct RImage {
  VkImage image;
  VkImageView view;
  VkFormat format;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
};

// expanding KTX structure
struct RVulkanTexture : public ktxVulkanTexture {
  VkImageView view;
  VkSampler sampler;
  VkDescriptorImageInfo descriptor;
};

// pipelines used by the 3D world
struct RWorldPipelineSet {
  VkPipeline PBR;           // standard PBR pipeline
  VkPipeline PBR_DS;        // double sided PBR pipeline
  VkPipeline skybox;        // depth independent skybox/sphere
  VkPipelineLayout layout;  // general 3D world pipeline layout
};

struct RVertex {
  glm::vec3 pos;        // POSITION
  glm::vec2 tex0;       // TEXCOORD0
  glm::vec2 tex1;       // TEXCOORD1
  glm::vec3 normal;     // NORMAL
  glm::vec4 color;      // COLOR
  glm::vec4 joint;      // JOINT
  glm::vec4 weight;     // WEIGHT

  static VkVertexInputBindingDescription getBindingDesc();
  static std::vector<VkVertexInputAttributeDescription> getAttributeDescs();
};

// stored by WModel, used to create a valid sampler for a specific texture
struct RSamplerInfo {
  VkFilter minFilter = VK_FILTER_LINEAR;
  VkFilter magFilter = VK_FILTER_LINEAR;
  VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};

// used for RMaterial creation in materials manager
struct RMaterialInfo {
  std::string name;
  bool manageTextures = false;
  bool doubleSided = false;
  ERAlphaMode alphaMode = ERAlphaMode::Opaque;
  float alphaCutoff = 1.0f;

  struct {
    std::string vertex = "default.vert", pixel = "default.frag", geometry = "";
  } shaders;

  struct {
    std::string baseColor = RE_DEFAULTTEXTURE;
    std::string normal = RE_NULLTEXTURE;
    std::string metalRoughness = RE_NULLTEXTURE;
    std::string occlusion = RE_NULLTEXTURE;
    std::string emissive = RE_NULLTEXTURE;
    std::string extra = RE_NULLTEXTURE;
  } textures;

  struct {
    uint8_t baseColor = 0;
    uint8_t normal = 0;
    uint8_t metalRoughness = 0;
    uint8_t occlusion = 0;
    uint8_t emissive = 0;
    uint8_t extra = 0;
  } texCoordSets;

  glm::vec4 F0 = {0.4f, 0.4f, 0.4f, 0.0f};  // basic metal
  glm::vec4 baseColorFactor = {0.0f, 0.0f, 0.0f, 1.0f};
  glm::vec4 emissiveFactor = {0.0f, 0.0f, 0.0f, 1.0f};
  float metallicFactor = 1.0f;
  float roughnessFactor = 1.0f;
  float bumpIntensity = 1.0f;
  float materialIntensity = 1.0f;

  uint32_t effectFlags = 0;
};

// push constant block used by RMaterial
struct RPushConstantBlock_Material {
  glm::vec4 baseColorFactor;
  glm::vec4 emissiveFactor;
  int32_t baseColorTextureSet;
  int32_t normalTextureSet;
  int32_t metallicRoughnessTextureSet;
  int32_t occlusionTextureSet;
  int32_t emissiveTextureSet;
  int32_t extraTextureSet;
  float metallicFactor;
  float roughnessFactor;
  float alphaMode;
  float alphaCutoff;
  float bumpIntensity;
  float materialIntensity;
};

struct RCameraSettings {
  float aspectRatio = config::getAspectRatio();
  float FOV = config::FOV;
  float nearZ = RE_NEARZ;
  float farZ = config::viewDistance;
};

// uniform buffer objects
// 
// camera and view matrix UBO for vertex shader
struct RModelViewProjectionUBO {
  alignas(16) glm::mat4 model = glm::mat4(1.0f);
  alignas(16) glm::mat4 view = glm::mat4(1.0f);
  alignas(16) glm::mat4 projection = glm::mat4(1.0f);
  alignas(16) glm::vec3 cameraPosition = glm::vec3(0.0f);
};

// mesh and joints transformation uniform buffer object
struct RMeshUBO {
  glm::mat4 meshMatrix = glm::mat4(1.0f);
  glm::mat4 jointMatrix[RE_MAXJOINTS]{};
  float jointCount = 0.0f;
};

// lighting data uniform buffer object
struct RLightingUBO {
  glm::vec4 lightDir;
  float exposure = 1.0f;      // 4.5f
  float gamma = 2.2f;
  float prefilteredCubeMipLevels;
  float scaleIBLAmbient = 1.0f;
  float debugViewInputs = 0;
  float debugViewEquation = 0;
};