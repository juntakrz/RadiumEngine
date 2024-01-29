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

enum class EComputeJob {
  Image
};

enum class EComputePipeline {
  Null,
  ImageLUT,
  ImageMipMap16f,
  ImageEnvIrradiance,
  ImageEnvFilter
};

enum class EDescriptorSetLayout {
  Scene,
  Material,
  MaterialEXT,
  PBRInput,
  Model,
  ComputeImage,
  Dummy
};

enum EDynamicRenderingPass : uint32_t {
  Null                = 0,
  Shadow              = 0b1,
  EnvSkybox           = 0b10,
  OpaqueCullBack      = 0b100,
  OpaqueCullNone      = 0b1000,
  MaskCullBack        = 0b10000,
  BlendCullNone       = 0b100000,
  Skybox              = 0b1000000,
  PBR                 = 0b10000000,
  Present             = 0b100000000
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

enum class ERenderPass {
  Null,
  Shadow,
  Deferred,
  Present
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
  std::string vertexShader;
  std::string fragmentShader;

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
    VkBool32 blendEnable = VK_FALSE;
    VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
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
  RDynamicRenderingPass* pRenderPass;
  std::string vertexShader;
  std::string fragmentShader;
  VkPipelineRenderingCreateInfo* pDynamicPipelineInfo = nullptr;
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
  VkFilter minFilter = VK_FILTER_LINEAR;
  VkFilter magFilter = VK_FILTER_LINEAR;
  VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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
  glm::vec4 lightLocations[RE_MAXLIGHTS]; // w is unused
  glm::vec4 lightColors[RE_MAXLIGHTS];    // alpha is intensity
  float lightCount = 0.0f;
  float exposure = 4.5f;
  float gamma = 2.2f;
  float prefilteredCubeMipLevels;
  float scaleIBLAmbient = 1.0f;
};

// Push constant block used by the scene fragment shader
// (72 bytes, 88 bytes total of 128 Vulkan spec)
struct RSceneFragmentPCB {
  int32_t baseColorTextureSet;
  int32_t normalTextureSet;
  int32_t metallicRoughnessTextureSet;
  int32_t occlusionTextureSet;            // 16
  int32_t emissiveTextureSet;
  int32_t extraTextureSet;
  float metallicFactor;
  float roughnessFactor;                  // 32
  float alphaMode;
  float alphaCutoff;
  float bumpIntensity;
  float emissiveIntensity;                // 48
  uint32_t samplerIndex[RE_MAXTEXTURES];  // 72
};

// Push constant block used by the scene vertex shader
// (16 bytes, 64 bytes total of 128 Vulkan spec)
struct RSceneVertexPCB {
  alignas(16) uint32_t cascadeIndex = 0u;
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