#pragma once

#include "vk_mem_alloc.h"
#include "config.h"

class AEntity;

enum class EActorType {  // actor type
  Base,
  Camera,
  Entity,     // unity for types below
  Pawn,       // can be controlled by player input
  Static      // part of the scene not meant to be controlled
};

enum EAlphaMode {
  Opaque,
  Mask,
  Blend
};

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

enum class EDescriptorSetLayout {
  Scene,
  Material,
  Mesh,
  Environment
};

enum EPipeline : uint32_t {
  Null = 0,
  Environment = 1,
  Depth = 2,
  Skybox = 4,
  OpaqueCullBack = 8,
  OpaqueCullNone = 16,
  MaskCullBack = 32,
  BlendCullBack = 64
};

enum class EPipelineLayout {
  Null,
  Scene
};

enum class EPrimitiveType {
  Null,
  Plane,
  Sphere,
  Cube,
  Custom
};

enum class ERenderPass {
  Null,
  Environment,
  PBR
};

struct RVkLogicalDevice {
  VkDevice device;

  struct {
    VkQueue graphics = VK_NULL_HANDLE;
    VkQueue compute = VK_NULL_HANDLE;
    VkQueue present = VK_NULL_HANDLE;
    VkQueue transfer = VK_NULL_HANDLE;
  } queues;
};

struct RVkSwapChainInfo {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> modes;
};

struct RVkQueueFamilyIndices {
  std::vector<int32_t> graphics;
  std::vector<int32_t> compute;
  std::vector<int32_t> present;
  std::vector<int32_t> transfer;

  // retrieve first entries for queue family indices
  std::set<int32_t> getAsSet() const;
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

class RAsync {
  std::thread thread;
  std::mutex mutex;
  std::condition_variable conditional;
  TFuncPtr func;
  bool cue = false;
  bool execute = true;

  void loop();

public:
  // example: bindFunction(this, &RClass::method)
  template <typename C>
  void bindFunction(C* owner, void (C::*function)()) {
    func = std::make_unique<OFuncPtr<C>>(owner, function);
  }

  // bind function before calling start()
  void start();

  // immediately stop this thread
  void stop();

  // execute bound function
  void update();
};

struct RBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
};

struct RRenderPass {
  VkRenderPass renderPass;
  std::vector<EPipeline> usedPipelines;
  VkPipelineLayout usedLayout;
  VkViewport viewport;
};

// expanding KTX structure
struct RVulkanTexture : public ktxVulkanTexture {
  VkImageView view;
  VkSampler sampler;
  VkDescriptorImageInfo descriptor;
};

struct RVertex {
  glm::vec3 pos;        // POSITION
  glm::vec3 normal;     // NORMAL
  glm::vec2 tex0;       // TEXCOORD0
  glm::vec2 tex1;       // TEXCOORD1
  glm::vec4 joint;      // JOINT
  glm::vec4 weight;     // WEIGHT
  glm::vec4 color;      // COLOR      aligned to 96 bytes per vertex on device

  static VkVertexInputBindingDescription getBindingDesc();
  static std::vector<VkVertexInputAttributeDescription> getAttributeDescs();
};

struct RCameraInfo {
  float aspectRatio = config::getAspectRatio();
  float FOV = config::FOV;
  float nearZ = RE_NEARZ;
  float farZ = config::viewDistance;
};

// used for RMaterial creation in materials manager
struct RMaterialInfo {
  std::string name = "default";
  bool manageTextures = false;
  bool doubleSided = false;
  EAlphaMode alphaMode = EAlphaMode::Opaque;
  float alphaCutoff = 1.0f;

  struct {
    std::string vertex = "default.vert", pixel = "default.frag", geometry = "";
  } shaders;

  struct {
    std::string baseColor = RE_DEFAULTTEXTURE;
    std::string normal = "";
    std::string metalRoughness = "";
    std::string occlusion = "";
    std::string emissive = "";
    std::string extra = "";
  } textures;

  struct {
    int8_t baseColor = 0;
    int8_t normal = 0;
    int8_t metalRoughness = 0;
    int8_t occlusion = 0;
    int8_t emissive = 0;
    int8_t extra = 0;
  } texCoordSets;

  glm::vec4 F0 = {0.04f, 0.04f, 0.04f, 0.0f};
  glm::vec4 baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
  glm::vec4 emissiveFactor = {0.0f, 0.0f, 0.0f, 1.0f};
  float metallicFactor = 1.0f;
  float roughnessFactor = 1.0f;
  float bumpIntensity = 1.0f;
  float materialIntensity = 1.0f;

  // if 'Null' - pipeline is determined using material properties
  uint32_t pipelineFlags = EPipeline::Null;
};

// used by createTexture()
struct RTextureInfo {
  std::string name = "";
  uint32_t width = 0u;
  uint32_t height = 0u;
  VkImageUsageFlags usageFlags;
  VkFormat format = core::vulkan::formatLDR;
  VkImageLayout targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  bool asCubemap = false;
  VkMemoryPropertyFlags memoryFlags = NULL;
  VmaMemoryUsage vmaMemoryUsage = VMA_MEMORY_USAGE_AUTO;
};

struct REntityBindInfo {
  AEntity* pEntity = nullptr;
  uint32_t vertexOffset = 0u;
  uint32_t indexOffset = 0u;
  uint32_t vertexCount = 0u;
  uint32_t indexCount = 0u;
};

struct RPrimitiveInfo {
  uint32_t vertexOffset = 0u;
  uint32_t indexOffset = 0u;
  uint32_t vertexCount = 0u;
  uint32_t indexCount = 0u;

  bool createTangentSpaceData = false;
  std::vector<RVertex>* pVertexData = nullptr;
  std::vector<uint32_t>* pIndexData = nullptr;
  void* pOwnerNode = nullptr;
};

// stored by WModel, used to create a valid sampler for a specific texture
struct RSamplerInfo {
  VkFilter minFilter = VK_FILTER_LINEAR;
  VkFilter magFilter = VK_FILTER_LINEAR;
  VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};

//
// uniform buffer objects and push contant blocks
// 

// camera and view matrix UBO for vertex shader
struct RSceneUBO {
  alignas(16) glm::mat4 view = glm::mat4(1.0f);
  alignas(16) glm::mat4 projection = glm::mat4(1.0f);
  alignas(16) glm::vec3 cameraPosition = glm::vec3(0.0f);
};

// mesh and joints transformation uniform buffer object
struct RMeshUBO {
  glm::mat4 rootMatrix = glm::mat4(1.0f);
  glm::mat4 nodeMatrix = glm::mat4(1.0f);
  glm::mat4 jointMatrix[RE_MAXJOINTS]{};
  float jointCount = 0.0f;
};

// lighting data uniform buffer object
struct RLightingUBO {
  glm::vec4 lightDir;
  float exposure = 4.5f;
  float gamma = 2.2f;
  float prefilteredCubeMipLevels;
  float scaleIBLAmbient = 1.0f;
};

// push constant block used by RMaterial
// (96 bytes of 128 Vulkan spec)
struct RMaterialPCB {
  glm::vec4 baseColorFactor;
  glm::vec4 emissiveFactor;
  glm::vec4 f0;
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