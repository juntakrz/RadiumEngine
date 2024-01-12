#pragma once

#include "vk_mem_alloc.h"
#include "config.h"

class ABase;
class AEntity;

enum class EActorType {  // actor type
  Base,
  Camera,
  Light,
  Entity,     // unity for types below
  Pawn,       // can be controlled by player input
  Static      // part of the scene not meant to be controlled
};

enum EAlphaMode {
  Opaque,
  Mask,
  Blend
};

enum EAnimationLoadMode {
  None,
  // load animations referenced by model from drive if not already loaded
  OnDemand,
  // extract animations from model to manager
  ExtractToManager,
  // extract animations from model to manager and storage
  ExtractToManagerAndStorage,
  // extract animations from model to storage only
  ExtractToStorageOnly
};

enum class EBufferType {  // VkBuffer creation mode
  NONE,
  STAGING,            // CPU staging buffer
  CPU_UNIFORM,        // uniform buffer for GPU programs
  CPU_VERTEX,         // vertex buffer for the iGPU (UNUSED)
  CPU_INDEX,          // index buffer for the iGPU (UNUSED)
  DGPU_VERTEX,        // dedicated GPU vertex buffer
  DGPU_INDEX,         // dedicated GPU index buffer
  DGPU_STORAGE,       // dedicated GPU storage buffer
  DGPU_SAMPLER,       // dedicated GPU storage buffer for sampler descriptors
  DGPU_RESOURCE,      // dedicated GPU storage buffer for resource descriptors
};

enum class ECameraProjection {
  Perspective,
  Orthogtaphic
};

enum class ECmdType {
  Graphics,
  Compute,
  Transfer,
  Present
};

enum class EComputePipeline {
  Null,
  ImageLUT,
  ImageMipMap16f
};

enum class EDescriptorSetLayout {
  Scene,
  Material,
  MaterialEXT,
  PBRInput,
  Model,
  Environment,
  ComputeImage,
  Dummy
};

enum class EDynamicRenderingPass {
  Null,
  Environment
};

enum class ELightType {
  Directional,
  Point
};

enum EPipeline : uint32_t {
  Null              = 0,
  EnvFilter         = 0b1,
  EnvIrradiance     = 0b10,
  Shadow            = 0b100,
  Skybox            = 0b1000,
  OpaqueCullBack    = 0b10000,
  OpaqueCullNone    = 0b100000,
  MaskCullBack      = 0b1000000,
  BlendCullNone     = 0b10000000,
  PBR               = 0b100000000,
  Present           = 0b1000000000,

  // combined pipeline indices for rendering only
  MixEnvironment = EnvFilter + EnvIrradiance
};

enum class EPipelineLayout {
  Null,
  Scene,
  PBR,
  Environment,
  Shadow,
  ComputeImage
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
  Shadow,
  Deferred,
  Present
};

enum class ETransformType { Translation, Rotation, Scale, Weight, Undefined };

enum EViewport { vpEnvFilter, vpEnvIrrad, vpShadow, vpMain, vpCount };  // 'Count' is a hack

struct RBuffer {
  EBufferType type = EBufferType::NONE;
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
  VkDeviceAddress deviceAddress = 0u;
};

struct RCameraInfo {
  float FOV = config::FOV;
  float aspectRatio = 1.0f; // ratio 1.0 corresponds to resolution
  float nearZ = RE_NEARZ;
  float farZ = config::viewDistance;
};

struct RComputePipelineInfo {
  EComputePipeline type;
};

struct RDynamicRenderingInfo {
  std::vector<struct RTexture*> pColorAttachments;
  struct RTexture* pDepthAttachment = nullptr;
  struct RTexture* pStencilAttachment = nullptr;
  VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct REntityBindInfo {
  AEntity* pEntity = nullptr;
  uint32_t vertexOffset = 0u;
  uint32_t indexOffset = 0u;
  uint32_t vertexCount = 0u;
  uint32_t indexCount = 0u;
};

struct RFramebuffer {
  VkFramebuffer framebuffer;
  std::vector<struct RTexture*> pFramebufferAttachments;
};

struct RGraphicsPipelineInfo {
  ERenderPass renderPass;
  EPipelineLayout pipelineLayout;
  EPipeline pipeline;
  EViewport viewportId;
  uint32_t colorBlendAttachmentCount;
  std::string vertexShader;
  std::string fragmentShader;

  EDynamicRenderingPass dynamicRenderPass = EDynamicRenderingPass::Null;
  uint32_t subpass = 0;
  VkBool32 blendEnable = VK_FALSE;
  VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
  VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
};

struct RLightInfo {
  ELightType type = ELightType::Point;
  glm::vec3 color = {1.0f, 1.0f, 1.0f};
  float intensity = 1.0f;
  glm::vec3 direction = {0.0f, 0.0f, 0.0f};   // used by directional light only
  glm::vec3 translation = {0.0f, 0.0f, 0.0f}; // used by point light only, both may be used by spotlight
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

  float metallicFactor = 0.0f;
  float roughnessFactor = 1.0f;
  float bumpIntensity = 1.0f;
  float emissiveIntensity = 0.0f;

  // if 'Null' - pipeline is determined using material properties
  uint32_t pipelineFlags = EPipeline::Null;
};

struct RPipeline {
  VkPipeline pipeline = VK_NULL_HANDLE;
  EPipeline pipelineId = EPipeline::Null;
  uint32_t subpassIndex = 0;

  struct {
    std::vector<VkRenderingAttachmentInfo> colorAttachmentInfo;
    std::vector<VkFormat> colorAttachmentFormats;
    VkRenderingAttachmentInfo depthAttachmentInfo;
    VkRenderingAttachmentInfo stencilAttachmentInfo;
    VkPipelineRenderingCreateInfo pipelineCreateInfo;
    std::vector<struct RTexture*> pColorAttachments;
    struct RTexture* pDepthAttachment = nullptr;
    struct RTexture* pStencilAttachment = nullptr;
  } dynamic;
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

struct RRenderPass {
  EDynamicRenderingPass dynamicPass = EDynamicRenderingPass::Null;  // if not 'Null' - then this is should be treated as a dynamic rendering pass
  VkRenderPass renderPass = nullptr;
  RFramebuffer* pFramebuffer = nullptr;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  std::vector<VkClearValue> clearValues;
  std::vector<RPipeline*> pipelines;

  RPipeline* getPipeline(EPipeline pipeline);
};

struct RRenderPassInfo {
  std::vector<VkAttachmentDescription> colorAttachmentInfo;
  VkAttachmentDescription depthAttachmentInfo;
  EPipelineLayout layout;
  uint32_t viewportWidth;
  uint32_t viewportHeight;
  std::vector<VkClearValue> clearValues;
  uint32_t subpassIndex;
};

// stored by WModel, used to create a valid sampler for a specific texture
struct RSamplerInfo {
  VkFilter minFilter = VK_FILTER_LINEAR;
  VkFilter magFilter = VK_FILTER_LINEAR;
  VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};

// used by createTexture()
struct RTextureInfo {
  std::string name = "";
  uint32_t width = 0u;
  uint32_t height = 0u;
  VkImageUsageFlags usageFlags;
  VkFormat format = core::vulkan::formatLDR;
  uint32_t layerCount = 1u;
  uint32_t mipLevels = 1u;
  VkImageLayout targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  bool asCubemap = false;
  bool detailedViews = false;
  VkMemoryPropertyFlags memoryFlags = NULL;
  VmaMemoryUsage vmaMemoryUsage = VMA_MEMORY_USAGE_AUTO;
};

struct RViewport {
  VkViewport viewport;
  VkRect2D scissor;
};

struct RVertex {
  glm::vec3 pos;     // POSITION
  glm::vec3 normal;  // NORMAL
  glm::vec2 tex0;    // TEXCOORD0
  glm::vec2 tex1;    // TEXCOORD1
  glm::vec4 joint;   // JOINT
  glm::vec4 weight;  // WEIGHT
  glm::vec4 color;   // COLOR      aligned to 96 bytes per vertex on device

  static VkVertexInputBindingDescription getBindingDesc();
  static std::vector<VkVertexInputAttributeDescription> getAttributeDescs();
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
  VkPhysicalDeviceFeatures2 deviceFeatures;
  VkPhysicalDeviceProperties2 deviceProperties;
  VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProperties;
  VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptorIndexingProperties;
  VkPhysicalDeviceMemoryProperties memProperties;
  RVkQueueFamilyIndices queueFamilyIndices;
  RVkSwapChainInfo swapChainInfo;
  bool bIsValid = false;
};

// expanding KTX structure
struct RVulkanTexture : public ktxVulkanTexture {
  VkImageView view;
  std::vector<std::vector<VkImageView>> layerAndMipViews;  // [layer][mip level]
  VkSampler sampler;
  VkDescriptorImageInfo descriptor;
};

//
// uniform buffer objects and push contant blocks
// 

struct RComputeImagePCB {
  uint32_t uint0;
};

struct REnvironmentFragmentPCB {
  float roughness;
  uint32_t samples;
};

// camera rotation UBO for environment map generation
struct REnvironmentUBO {
  glm::mat4 view;
  glm::mat4 projection;
};

// lighting data uniform buffer object
struct RLightingUBO {
  glm::vec4 lightLocations[RE_MAXLIGHTS]; // w is unused
  glm::vec4 lightColors[RE_MAXLIGHTS];    // alpha is intensity
  float lightCount = 0.0f;
  float exposure = 4.5f;
  float gamma = 2.2f;
  float prefilteredCubeMipLevels;
  float scaleIBLAmbient = 1.0f;
};

// Push constant block used by the scene fragment shader
// (48 bytes, 64 bytes total of 128 Vulkan spec)
struct RSceneFragmentPCB {
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
  float emissiveIntensity;
  int32_t materialIndex;
};

// Push constant block used by the scene vertex shader
// (16 bytes, 64 bytes total of 128 Vulkan spec)
struct RSceneVertexPCB {
  VkDeviceAddress intoVertexBufferAddress = 0u;
  uint32_t cascadeIndex = 0u;
  uint32_t padding = 0u;
};

struct RMeshUBO {
  glm::mat4 nodeMatrix = glm::mat4(1.0f);
  float jointCount = 0.0f;
  std::vector<glm::mat4> jointMatrices;
};

// camera and view matrix UBO for vertex shader
struct RSceneUBO {
  alignas(16) glm::mat4 view = glm::mat4(1.0f);
  alignas(16) glm::mat4 projection = glm::mat4(1.0f);
  alignas(16) glm::vec3 cameraPosition = glm::vec3(0.0f);
  VkDeviceAddress vertexBufferAddress = 0u;
};

struct WAnimationInfo {
  class WModel* pModel;
  std::string animationName;
  float startTime = 0.0f;
  float endTime = std::numeric_limits<float>::max();
  float speed = 1.0f;
  bool loop = true;
  bool bounce = false;
};

struct WAttachmentInfo {
  ABase* pAttached = nullptr;
  glm::vec3 vector = glm::vec3(0.0f);
  bool attachTranslation = true;
  bool attachRotation = true;
  bool attachToForwardVector = false;
};

struct WModelConfigInfo {
  // see EAnimationLoadMode definition for information
  EAnimationLoadMode animationLoadMode = EAnimationLoadMode::OnDemand;
  // skeleton name/folder to use for loading/saving if an appropriate load mode is used
  std::string skeleton = "default";
  // framerate to sample animations at if extracting
  float framerate = 15.0f;
  // speed up extracted animations while sampling, will apply to all
  float speed = 1.0f;
};