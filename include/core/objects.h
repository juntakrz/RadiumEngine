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
  CPU_UNIFORM,        // Uniform buffer for GPU programs
  CPU_VERTEX,         // Vertex buffer for the iGPU (UNUSED)
  CPU_INDEX,          // Index buffer for the iGPU (UNUSED)
  CPU_STORAGE,        // Storage buffer for CPU to write and read data from
  DGPU_VERTEX,        // Dedicated GPU vertex buffer
  DGPU_INDEX,         // Dedicated GPU index buffer
  DGPU_UNIFORM,
  DGPU_STORAGE,       // Dedicated GPU storage buffer
  DGPU_SAMPLER,       // Dedicated GPU storage buffer for sampler descriptors
  DGPU_RESOURCE,      // Dedicated GPU storage buffer for resource descriptors
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

enum class EComputeJob {
  Image
};

enum class EComputePipeline {
  Null,
  ImageLUT,
  ImageEnvIrradiance,
  ImageEnvFilter
};

enum class EControlMode {
  Cursor,
  MouseLook
};

enum class EDescriptorSetLayout {
  Scene,
  MaterialEXT,
  Model,
  ComputeImage,
  Dummy
};

enum EDynamicRenderingPass : uint32_t {
  Null                = 0,
  Shadow              = 0b1,
  ShadowDiscard       = 0b10,
  EnvSkybox           = 0b100,
  OpaqueCullBack      = 0b1000,
  OpaqueCullNone      = 0b10000,
  DiscardCullNone     = 0b100000,
  MaskCullBack        = 0b1000000,
  BlendCullNone       = 0b10000000,
  Skybox              = 0b100000000,
  AlphaCompositing    = 0b1000000000,
  PBR                 = 0b10000000000,
  PPDownsample        = 0b100000000000,
  PPUpsample          = 0b1000000000000,
  PPGetExposure       = 0b10000000000000,
  PPTAA               = 0b100000000000000,
  Present             = 0b1000000000000000
};

enum class ELightType {
  Directional,
  Point
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

enum class EResourceType {
  Null,
  Sampler2D,
  Image2D
};

enum class ETransformType { Translation, Rotation, Scale, Weight, Undefined };

enum EViewport { vpEnvironment, vpEnvIrrad, vpShadow, vpMain, vpCount };  // 'Count' is a hack

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

struct RComputeJobInfo {
  EComputeJob jobType;
  EComputePipeline pipeline;
  std::vector<struct RTexture*> pImageAttachments;
  std::vector<struct RTexture*> pSamplerAttachments;
  uint32_t width, height, depth = 1;
  bool transtionToShaderReadOnly;
  bool useExtraImageViews = false;
  bool useExtraSamplerViews = false;
  glm::ivec4 intValues = glm::ivec4(0);
  glm::vec4 floatValues = glm::vec4(0.0f);
};

struct RDynamicAttachmentInfo {
  RTexture* pImage = nullptr;
  VkImageView view = VK_NULL_HANDLE;
  VkFormat format;
};

struct RDynamicRenderingInfo {
  EPipelineLayout pipelineLayout;
  EViewport viewportId;
  std::vector<RDynamicAttachmentInfo> colorAttachments;
  RDynamicAttachmentInfo depthAttachment;
  RDynamicAttachmentInfo stencilAttachment;
  VkImageLayout depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  VkClearValue colorAttachmentClearValue = {0.0f, 0.0f, 0.0f, 0.0f};
  bool singleColorAttachmentAtRuntime = false;
  bool clearColorAttachments = false;
  bool clearDepthAttachment = false;
  std::string vertexShader = "";
  std::string geometryShader = "";
  std::string fragmentShader = "";

  struct {
    // Transition image layouts upon rendering if expected that they are not going to be valid
    bool validateColorAttachmentLayout = false;
    bool validateDepthAttachmentLayout = false;

    // Transition image layouts upon finishing this pass to specified layout
    bool transitionColorAttachmentLayout = false;
    VkImageLayout colorAttachmentsOutLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bool transitionDepthAttachmentLayout = false;
  } layoutInfo;

  struct {
    VkBool32 enableBlending = VK_FALSE;
    VkBool32 enableDepthWrite = VK_TRUE;
    VkBool32 enableDepthTest = VK_TRUE;
    VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;

    struct {
      VkBool32 enable = VK_FALSE;
      float constantFactor = 1.75f;
      float slopeFactor = 1.25f;
      float clamp = 0.0f;
    } depthBias;
  } pipelineInfo;
};

struct RDynamicRenderingPass {
  EDynamicRenderingPass passId;
  EViewport viewportId;
  EPipelineLayout layoutId;

  VkRenderingInfo renderingInfo;
  std::vector<VkRenderingAttachmentInfo> colorAttachments;
  VkRenderingAttachmentInfo depthAttachment;
  VkRenderingAttachmentInfo stencilAttachment;
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  std::vector<RTexture*> pImageReferences;
  uint32_t colorAttachmentCount = 0u;

  bool validateColorAttachmentLayout = false;
  bool validateDepthAttachmentLayout = false;
  bool transitionColorAttachmentLayout = false;
  VkImageLayout colorAttachmentsOutLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  bool transitionDepthAttachmentLayout = false;
};

struct REntityBindInfo {
  AEntity* pEntity = nullptr;
};

struct RFramebuffer {
  VkFramebuffer framebuffer;
  std::vector<struct RTexture*> pFramebufferAttachments;
};

struct RGraphicsPipelineInfo {
  RDynamicRenderingPass* pRenderPass;
  std::string vertexShader;
  std::string fragmentShader;
  std::string geometryShader;
  VkPipelineRenderingCreateInfo* pDynamicPipelineInfo = nullptr;
  VkBool32 enableBlending = VK_FALSE;
  VkBool32 enableDepthWrite = VK_TRUE;
  VkBool32 enableDepthTest = VK_TRUE;
  VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
  VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;

  struct {
    VkBool32 enable = VK_FALSE;
    float constantFactor = 0.0f;
    float slopeFactor = 0.0f;
    float clamp = 0.0f;
  } depthBias;
};

// References into three transform buffers
struct RInstanceData {
  int32_t modelMatrixId = -1;
  int32_t nodeMatrixId = -1;
  int32_t skinMatrixId = -1;
  int32_t materialId = -1;
};

struct RLightInfo {
  ELightType type = ELightType::Point;
  glm::vec3 color = {1.0f, 1.0f, 1.0f};
  float intensity = 1.0f;
  glm::vec3 direction = {0.0f, 0.0f, 0.0f};   // used by directional light only
  glm::vec3 translation = {0.0f, 0.0f, 0.0f}; // used by point light only, both may be used by spotlight
  bool isShadowCaster = false;
};

// used for RMaterial creation in materials manager
struct RMaterialInfo {
  std::string name = "default";
  bool manageTextures = false;
  bool doubleSided = false;
  EAlphaMode alphaMode = EAlphaMode::Opaque;
  float alphaCutoff = 1.0f;

  struct {
    std::string baseColor = RE_DEFAULTTEXTURE;
    std::string normal = "";
    std::string metalRoughness = "";
    std::string occlusion = "";
    std::string emissive = "";
    std::string extra0 = "";
    std::string extra1 = "";
    std::string extra2 = "";
  } textures;

  struct {
    int8_t baseColor = 0;
    int8_t normal = 0;
    int8_t metalRoughness = 0;
    int8_t occlusion = 0;
    int8_t emissive = 0;
    int8_t extra0 = 0;
    int8_t extra1 = 0;
    int8_t extra2 = 0;
  } texCoordSets;

  float metallicFactor = 0.0f;
  float roughnessFactor = 1.0f;
  float bumpIntensity = 1.0f;
  glm::vec4 glowColor = glm::vec4(0.0f);

  // if 'Null' - pipeline is determined using material properties
  uint32_t passFlags = EDynamicRenderingPass::Null;
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
  VkFilter filter = VK_FILTER_LINEAR;
  VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};

// used by createTexture()
struct RTextureInfo {
  std::string name;
  uint32_t width;
  uint32_t height;
  uint32_t layerCount;
  uint32_t mipLevels = 1u;
  VkImageUsageFlags usageFlags;
  VkFormat format = core::vulkan::formatLDR;
  VkImageLayout targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  bool isCubemap = false;
  bool cubemapFaceViews = false;
  bool extraViews = false;  // Create views into layers and mip levels
  VkMemoryPropertyFlags memoryFlags = NULL;
  VmaMemoryUsage vmaMemoryUsage = VMA_MEMORY_USAGE_AUTO;
  RSamplerInfo samplerInfo = RSamplerInfo{};
};

struct RTransparencyLinkedListData {
  uint32_t nodeCount;
  uint32_t maxNodeCount;
};

// An OIT linked node
struct RTransparencyLinkedListNode {
  glm::vec4 color;
  float depth;
  uint32_t nextNodeIndex;
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

  static std::vector<VkVertexInputBindingDescription> getBindingDescs();
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
  std::vector<VkImageView> cubemapFaceViews;
  std::vector<VkDescriptorImageInfo> extraViews;
  VkSampler sampler;
  VkDescriptorImageInfo imageInfo;
  VkImageAspectFlags aspectMask;
};

//
// uniform buffer objects and push contant blocks
// 

struct RComputeImagePCB {
  uint32_t imageIndex;
  uint32_t imageCount = 0;
  glm::ivec4 intValues = glm::ivec4(0);
  glm::vec4 floatValues = glm::vec4(0.0f);
};

// lighting data uniform buffer object
struct RLightingUBO {
  glm::vec4 lightLocations[RE_MAXLIGHTS];     // w is unused
  glm::vec4 lightColors[RE_MAXLIGHTS];        // alpha is intensity
  glm::mat4 lightViews[RE_MAXSHADOWCASTERS];  // 0 - directional, 1 - 5 point light reserved
  glm::mat4 lightOrthoMatrix;                 // default orthogonal projection matrix for light views
  uint32_t samplerArrayIndex[RE_MAXSHADOWCASTERS];
  uint32_t lightCount = 0;
  glm::vec4 shadowColor = {0.0f, 0.0f, 0.0f, 1.0f};
  float averageLuminance = 0.5f;
  float bloomIntensity = 1.0f;
  float exposure = 4.5f;
  float gamma = 2.2f;
  float prefilteredCubeMipLevels;
  float scaleIBLAmbient = 1.0f;
};

// Push constant block used by the scene fragment shader
// (64 bytes, 80 bytes total of 128 Vulkan spec)
struct RSceneFragmentPCB {
  int32_t textureSets = 0;                // Each 2 bits store a UV index 0, 1 or none
  float metallicFactor;
  float roughnessFactor;
  float alphaMode;
  float alphaCutoff;
  float bumpIntensity;
  uint32_t samplerIndex[RE_MAXTEXTURES];
  glm::vec4 glowColor;
};

// Push constant block used by the scene vertex shader
// (16 bytes, 64 bytes total of 128 Vulkan spec)
struct RSceneVertexPCB {
  uint32_t cascadeIndex = 0u;
  float padding[3];
};

struct RModelUBO {
  glm::mat4 modelMatrix = glm::mat4(1.0f);
};

struct RNodeUBO {
  glm::mat4 nodeMatrix = glm::mat4(1.0f);
  float jointCount = 0.0f;
};

// camera and view matrix UBO for vertex shader
struct RSceneUBO {
  alignas(16) glm::mat4 view = glm::mat4(1.0f);
  alignas(16) glm::mat4 projection = glm::mat4(1.0f);
  alignas(16) glm::vec3 cameraPosition = glm::vec3(0.0f);
  alignas(16) glm::vec2 haltonJitter = glm::vec2(0.0f);
  glm::vec2 clipData = glm::vec2(0.0f);
};

struct RSkinUBO {
  std::vector<glm::mat4> jointMatrices;
};

struct WAnimationInfo {
  class AEntity* pEntity;
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

struct WPrimitiveInstanceData {
  uint32_t instanceIndex = 0;
  uint32_t passFlags = 0;
  bool isVisible = true;

  RInstanceData instanceBufferBlock;
};