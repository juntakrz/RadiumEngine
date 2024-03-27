#pragma once

#include "vk_mem_alloc.h"
#include "core/objects.h"
#include "core/async.h"
#include "common.h"
#include "core/world/actors/camera.h"

class WModel;
class WPrimitive;
struct RTexture;
struct RMaterial;

namespace core {

class MRenderer {
  friend class MGUI;

 private:
  // command buffers and pools data
  struct {
    VkCommandPool poolGraphics;
    VkCommandPool poolCompute;
    VkCommandPool poolTransfer;
    std::vector<VkCommandBuffer> buffersGraphics;
    std::vector<VkCommandBuffer> buffersCompute;
    std::vector<VkCommandBuffer> buffersTransfer;

    std::vector<VkDrawIndexedIndirectCommand> indirectCommands;
    std::vector<RBuffer> indirectCommandBuffers;
    VkDeviceSize indirectCommandOffset = 0u;
  } command;

  struct {
    VkDescriptorSet imageDescriptorSet;
    VkDescriptorSet bufferDescriptorSet;
    uint32_t maxBoundDescriptorSets = 0u;

    // Used to track and modify relative indices when executing jobs
    uint32_t freeImageIndex = 0;
    uint32_t freeSamplerIndex = 0;
    uint32_t freeBufferIndex = 0;

    std::vector<RComputeJobInfo> queuedJobs;

    struct {
      RComputeJobInfo LUT;
      RComputeJobInfo irradiance;
      RComputeJobInfo prefiltered;
    } environmentJobInfo;

    struct {
      RComputeJobInfo culling;
    } sceneJobInfo;

    struct {
      RComputeJobInfo mipmapping;
    } imageJobInfo;
  } compute;

  struct REnvironmentData {
    VkDescriptorSet LUTDescriptorSet;
    VkImageSubresourceRange subresourceRange;
    int32_t genInterval = 2;
    RTexture* pTargetCubemap = nullptr;
    std::array<glm::vec3, 6> cameraTransformVectors;

    struct {
      uint32_t layer = 0;     // cubemap layer
    } tracking;
  } environment;

  struct RGUIData {
    VkRenderingAttachmentInfo colorAttachment;
    VkRenderingInfo renderingInfo;
  } gui;

  struct RLightingData {
    std::vector<RBuffer> buffers;
    RLightingUBO data;
  } lighting;

  struct RMaterialData {
    RBuffer buffer;
    VkDescriptorSet descriptorSet;
    RMaterial* pSunShadow = nullptr;
    RMaterial* pGBuffer = nullptr;
    RMaterial* pGPBR = nullptr;
    RMaterial* pBlur = nullptr;

    std::vector<VkSampler> samplers;
  } material;

  struct RSceneBuffers {
    RBuffer vertexBuffer;
    RBuffer indexBuffer;
    RBuffer modelTransformBuffer;
    RBuffer nodeTransformBuffer;
    RBuffer skinTransformBuffer;
    RBuffer generalBuffer;
    uint32_t currentVertexOffset = 0u;
    uint32_t currentIndexOffset = 0u;
    size_t totalInstances = 0u;
    VkDescriptorSet transformDescriptorSet;

    // Indirect draw data
    RBuffer sourceDataBuffer;                         // Contains all bound primitive instance data, changes when new entity is bound/unbound
    std::vector<RBuffer> instanceDataBuffers;
    std::vector<RBuffer> drawIndirectBuffers;
    std::vector<RBuffer> drawCountBuffers;
    uint32_t drawCounts[MAX_FRAMES_IN_FLIGHT][(uint32_t)EIndirectPassIndex::Count];
    uint32_t drawOffsets[MAX_FRAMES_IN_FLIGHT][(uint32_t)EIndirectPassIndex::Count];     // Draw counts and command offsets for each render pass
    uint32_t nextPrimitiveUID = 0;                    // Earliest free primitive UID
    uint32_t maxPrimitiveUID = 0;                     // Latest primitive UID / max limit for searching primitives by UID
    uint32_t nextInstanceUID = 0;
    uint32_t maxInstanceUID = 0;

    std::vector<RTexture*> pDepthTargets;
    std::vector<RTexture*> pPreviousDepthTargets;
    std::vector<RTexture*> pGBufferTargets;

    RBuffer transparencyLinkedListBuffer;
    RBuffer transparencyLinkedListDataBuffer;
    RTransparencyLinkedListData transparencyLinkedListData;
    RTexture* pTransparencyStorageTexture = nullptr;
    VkImageSubresourceRange transparencySubRange;
    VkClearColorValue transparencyLinkedListClearColor;

    RBuffer generalHostBuffer;

    // Per frame in flight buffered camera/lighting descriptor sets
    std::vector<VkDescriptorSet> descriptorSets;

    RSceneVertexPCB vertexPushBlock;
    
    std::vector<RBuffer> sceneBuffers;
    RSceneUBO sceneBufferObject;

    std::unordered_set<WModel*> pModelReferences;

    std::vector<RInstanceData> instanceData;
  } scene;

  struct RPostProcessData {
    RTexture* pBloomTexture = nullptr;
    RTexture* pExposureTexture = nullptr;
    RTexture* pPreviousFrameTexture = nullptr;
    RTexture* pTAATexture = nullptr;

    VkImageSubresourceRange bloomSubRange;
    std::vector<VkViewport> viewports;
    std::vector<VkRect2D> scissors;

    VkImageCopy previousFrameCopy;

    RBuffer exposureStorageBuffer;
  } postprocess;

  // swapchain data
  struct {
    VkSurfaceFormatKHR formatData;
    VkPresentModeKHR presentMode;
    VkExtent2D imageExtent;
    std::vector<RTexture*> pImages;
    uint32_t imageCount = 0u;
    VkImageCopy copyRegion;
  } swapchain;

  // multi-threaded synchronization objects
  struct {
    std::vector<VkSemaphore> semImgAvailable;
    std::vector<VkSemaphore> semRenderFinished;
    std::vector<VkFence> fenceInFlight;
    std::vector<VkFence> fenceUpdateBuffers;
    RAsync asyncUpdateEntities;
    RAsync asyncUpdateIndirectDrawBuffers;

    std::condition_variable cvInstanceDataReady;
    bool isInstanceDataReady = false;
    bool isTransformDataReady = false;
  } sync;

  // render system data - passes, pipelines, mesh data to render
  struct {
    std::unordered_map<EDynamicRenderingPass, RDynamicRenderingPass> dynamicRenderingPasses;
    std::unordered_map<EPipelineLayout, VkPipelineLayout> layouts;
    std::unordered_map<EComputePipeline, VkPipeline> computePipelines;
    std::vector<RViewport> viewports;
    std::vector<glm::vec2> haltonJitter;
    std::vector<glm::vec4> occlusionOffsets;
    std::vector<glm::vec4> randomOffsets;

    VkDescriptorPool descriptorPool;
    std::unordered_map<EDescriptorSetLayout, VkDescriptorSetLayout> descriptorSetLayouts;

    std::vector<REntityBindInfo> bindings;  // entities that are bound and will be rendered

    VkQueryPool queryPool;

    // NVidia GPUs seem to be using general layouts for everything on the driver level
    // so transitioning pipeline barriers may slow down performance in most cases
    bool enableLayoutTransitions = false;

    bool asyncComputeSupport = false;
    int32_t computeQueue = -1;
  } system;

  // current camera view data
  struct {
    RCameraInfo cameraSettings;
    WCameraComponent* pActiveCamera = nullptr;
    WCameraComponent* pPrimaryCamera = nullptr;
    WCameraComponent* pSunCamera = nullptr;
    glm::ivec2 raycastTarget;
    glm::mat4 previousCameraView = glm::mat4(1.0f);
  } view;

 public:
  VkInstance APIInstance = VK_NULL_HANDLE;
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  std::vector<RVkPhysicalDevice> availablePhysicalDevices;
  RVkPhysicalDevice physicalDevice;
  RVkLogicalDevice logicalDevice;
  VmaAllocator memAlloc = nullptr;
  bool framebufferResized = false;

  // data for easy access by any other object
  struct {
    void* pCurrentMaterial = nullptr;

    RDynamicRenderingPass* pCurrentPass = nullptr;
    VkDescriptorSet pCurrentSet = nullptr;
    EViewport currentViewportId = EViewport::vpCount;

    uint32_t currentFrameIndex = 0;
    uint32_t frameInFlight = 0;
    uint32_t framesRendered = 0;
    float lastFrameTime = 0.0f;
    bool generateEnvironmentMapsImmediate = false;  // queue single pass environment map gen (slow)
    bool generateEnvironmentMaps = false;           // queue sequenced environment map gen (fast)
    bool isEnvironmentPass = false;                 // is in the process of generating

    int32_t selectedActorUID = -1;

    void refresh() {
      pCurrentMaterial = nullptr;
    }
  } renderView;

 private:
  MRenderer();

  TResult createInstance();
  TResult createInstance(VkApplicationInfo* appInfo);
  TResult destroyInstance();

  // initialize Vulkan memory allocator
  TResult createMemAlloc();
  void destroyMemAlloc();

  // create Vulkan surface in the main window
  TResult createSurface();
  void destroySurface();

  TResult createSceneBuffers();
  void destroySceneBuffers();

  TResult createImageTargets();
  TResult createGBufferRenderTargets();
  TResult createDepthTargets();
  TResult setDefaultComputeJobs();
  TResult setRendererDefaults();

  TResult createCoreCommandPools();
  void destroyCoreCommandPools();

  TResult createCoreCommandBuffers();
  void destroyCoreCommandBuffers();

  TResult createSyncObjects();
  void destroySyncObjects();

  void updateSceneUBO(uint32_t currentImage);

  // wait until all queues and device are idle
  void waitForSystemIdle();

 public:
  static MRenderer& get() {
    static MRenderer _sInstance;
    return _sInstance;
  }
  MRenderer(const MRenderer&) = delete;
  MRenderer& operator=(const MRenderer&) = delete;

  TResult initialize();
  void deinitialize();

  // returns descriptor set used by the current frame in flight by default
  const VkDescriptorSet getSceneDescriptorSet(uint32_t frameInFlight = -1);

  RLightingData* getLightingData();
  VkDescriptorSet getMaterialDescriptorSet();
  RMaterialData* getMaterialData();
  RPostProcessData* getPostProcessingData();
  RSceneBuffers* getSceneBuffers();
  RSceneUBO* getSceneUBO();
  REnvironmentData* getEnvironmentData();
  bool isLayoutTransitionEnabled();

  void updateLightingUBO(const int32_t frameIndex);

  void updateAspectRatio();
  void setFOV(float FOV);
  void setViewDistance(float farZ);
  void setViewDistance(float nearZ, float farZ);

  //
  // ***DESCRIPTORS
  //

  // TODO: improve pool size calculations using loaded map data
  TResult createDescriptorPool();
  void destroyDescriptorPool();
  const VkDescriptorPool getDescriptorPool();

  TResult createDescriptorSetLayouts();
  void destroyDescriptorSetLayouts();

  const VkDescriptorSetLayout getDescriptorSetLayout(
      EDescriptorSetLayout type) const;

  TResult createDescriptorSets();

  //
  // ***PIPELINE
  //
  TResult createPipelineLayouts();
  VkPipelineLayout& getPipelineLayout(EPipelineLayout type);

  TResult createComputePipelines();
  void destroyComputePipelines();
  TResult createGraphicsPipeline(RGraphicsPipelineInfo* pipelineInfo);
  VkPipeline& getComputePipeline(EComputePipeline type);

  // check if pass flag is present in the pass array
  bool checkPass(uint32_t passFlags, EDynamicRenderingPass passFlag);

  //
  // ***PASS
  //

  // Create new dynamic rendering pass and/or add new/update existing attached pipeline
  TResult createDynamicRenderingPass(EDynamicRenderingPass passId, RDynamicRenderingInfo* pInfo);
  TResult createDynamicRenderingPasses();
  RDynamicRenderingPass* getDynamicRenderingPass(EDynamicRenderingPass type);
  void destroyDynamicRenderingPasses();

  //
  // ***UTIL
  //

 private:
  // create single layer render target for fragment shader output, uses swapchain resolution unless defined
  RTexture* createFragmentRenderTarget(const char* name, VkFormat format, uint32_t width = 0, uint32_t height = 0,
    VkSamplerAddressMode addressMode = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

  TResult createViewports();
  void setViewport(VkCommandBuffer commandBuffer, EViewport index);
  RViewport* getViewportData(EViewport viewportId);

  void setResourceName(VkDevice device, VkObjectType objectType,
                       uint64_t handle, const char* name);

  TResult getDepthStencilFormat(VkFormat desiredFormat, VkFormat& outFormat);

  VkPipelineShaderStageCreateInfo loadShader(const char* path,
                                             VkShaderStageFlagBits stage);
  VkShaderModule createShaderModule(std::vector<char>& shaderCode);

  TResult checkInstanceValidationLayers();
  std::vector<const char*> getRequiredInstanceExtensions();
  std::vector<VkExtensionProperties> getInstanceExtensions(const char* layerName = nullptr,
                                                           const char* extensionToCheck = nullptr,
                                                           bool* pCheckResult = nullptr);

  TResult createQueryPool();
  void destroyQueryPool();

  void updateBoundEntities();
  void updateExposureLevel();

public:
  TResult copyImage(VkCommandBuffer cmdBuffer, RTexture* pSrcTexture,
                    RTexture* pDstTexture, VkImageCopy& copyRegion);

  // Transition image to the desired layout, will not execute if an image is already transitioned unless forced
  void setImageLayout(VkCommandBuffer cmdBuffer, RTexture* pTexture,
                      VkImageLayout newLayout,
                      VkImageSubresourceRange subresourceRange,
                      const bool force = false);

  // Transition image to the desired layout, will not execute if an image is already transitioned unless forced
  void setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                      VkImageLayout oldLayout, VkImageLayout newLayout,
                      VkImageSubresourceRange subresourceRange,
                      const bool force = false);

  void convertRenderTargets(VkCommandBuffer cmdBuffer,
                            std::vector<RTexture*>* pInTextures,
                            bool convertBackToRenderTargets);

  TResult generateMipMaps(VkCommandBuffer cmdBuffer, RTexture* pTexture,
                          int32_t mipLevels, VkFilter filter = VK_FILTER_LINEAR);

  // generates a single mip map from a previous level at a set layer
  TResult generateSingleMipMap(VkCommandBuffer cmdBuffer, RTexture* pTexture,
                               uint32_t mipLevel, uint32_t layer = 0,
                               VkFilter filter = VK_FILTER_LINEAR);

  TResult createDefaultSamplers();
  void destroySamplers();
  VkSampler getSampler(RSamplerInfo* pInfo);

  VkCommandPool getCommandPool(ECmdType type);
  VkQueue getCommandQueue(ECmdType type);

  // creates a command buffer, if 'begin' is true - sets a "one time submit"
  // mode
  VkCommandBuffer createCommandBuffer(ECmdType type, VkCommandBufferLevel level,
                                      bool begin);

  // begins command buffer in a "one time submit" mode
  void beginCommandBuffer(VkCommandBuffer cmdBuffer, bool oneTimeSubmit = true);

  // end writing to the buffer and submit commands to specific queue,
  // optionally free the buffer and/or use fence
  void flushCommandBuffer(VkCommandBuffer cmdBuffer, ECmdType type,
                          bool free = false, bool useFence = false);

  // using isCubemap will override baseLayer and layerCount
  VkImageView createImageView(VkImage image, VkFormat format, uint32_t baseLayer,
    uint32_t layerCount, uint32_t baseLevel, uint32_t levelCount, const bool isCubemap, VkImageAspectFlags aspectMask);

  // binds model to graphics pipeline
  uint32_t bindEntity(AEntity* pEntity);

  // unbind primitive by a single index
  void unbindEntity(uint32_t index);

  // clear all primitive bindings
  void clearBoundEntities();

  const std::unordered_set<WModel*>& getModelReferences();

  void uploadModelToSceneBuffer(WModel* pModel);

  uint32_t getNewPrimitiveUID();

  void setRaycastPosition(const glm::ivec2& newTarget);
  const glm::ivec2& getRaycastPosition();

  void setSelectedActorUID(const int32_t actorUID);
  int32_t getSelectedActorUID();

  // set camera from create cameras by name
  void setCamera(ABase* pCameraOwner, const bool setAsPrimary = false);
  void setCamera(WCameraComponent* pCamera, const bool setAsPrimary = false);
  void setSunCamera(ABase* pCameraOwner);
  void setSunCamera(WCameraComponent* pCamera);

  // get current renderer camera
  WCameraComponent* getCamera();

  void setIBLScale(float newScale);
  void setShadowColor(const glm::vec3& color);
  void setBloomIntensity(const float intensity);

  float getFPS();
  float getFrameTime();

  //
  // ***BUFFER
  //

 public:
   /* create buffer for CPU/iGPU or dedicated GPU use:
   defining inData makes the method copy data to an outgoing buffer internally,
   otherwise empty but allocated VkBuffer is the result e.g. for a later data
   copy.
   */
   TResult createBuffer(EBufferType type, VkDeviceSize size, RBuffer& outBuffer,
     void* inData);

   // copy buffer with SRC and DST bits, uses transfer command buffer and pool
   TResult copyBuffer(VkBuffer srcBuffer, VkBuffer& dstBuffer,
     VkBufferCopy* copyRegion, uint32_t cmdBufferId = 0);
   TResult copyBuffer(RBuffer* srcBuffer, RBuffer* dstBuffer,
     VkBufferCopy* copyRegion, uint32_t cmdBufferId = 0);

   // expects 'optimal layout' image as a source
   TResult copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, RTexture* pDstImage,
                             uint32_t width, uint32_t height, uint32_t layerCount);

   TResult copyImageToBuffer(VkCommandBuffer commandBuffer, RTexture *pSrcImage, VkBuffer dstBuffer,
                             uint32_t width, uint32_t height, VkImageSubresourceLayers* subresource);

   void copyDataToBuffer(void* pData, VkDeviceSize dataSize, RBuffer* pDstBuffer, VkDeviceSize offset = 0u);

   void updateIndirectDrawBuffers();

  //
  // ***PHYSICAL DEVICE
  //

 public:
  RVkPhysicalDevice& getPhysicalDevice();

  // find all available physical devices and store them in graphics manager
  TResult enumPhysicalDevices();

  // automatically use the first valid physical device
  TResult initPhysicalDevice();

  // try to use this physical device if valid
  TResult initPhysicalDevice(const RVkPhysicalDevice& physicalDeviceData);

  // setup physical device database, validate all devices as per usage
  // requirements
  TResult setPhysicalDeviceData(VkPhysicalDevice device,
                                RVkPhysicalDevice& outDeviceData);

  // access currently detected devices
  std::vector<RVkPhysicalDevice>& physicalDevices();
  RVkPhysicalDevice* physicalDevices(uint32_t id = 0);

  // provides surface and presentation data for Vulkan swapchain
  TResult queryPhysicalDeviceSwapChainInfo(const RVkPhysicalDevice& deviceData,
                                           RVkSwapChainInfo& swapChainInfo);

  // find suitable memory type for the required operation on an active physical
  // device
  uint32_t findPhysicalDeviceMemoryType(uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties);

 private:
  TResult checkPhysicalDeviceExtensionSupport(
      const RVkPhysicalDevice& deviceData);

  std::vector<VkExtensionProperties> getPhysicalDeviceExtensions(
      const RVkPhysicalDevice& deviceData);

  // retrieve queue capabilities for the device, use only first valid indices
  TResult setPhysicalDeviceQueueFamilies(RVkPhysicalDevice& deviceData);

  //
  // ***LOGICAL DEVICE
  //

 public:
  // creates logical device from the currently active physical one
  TResult initLogicalDevice();

  // create a logical device to communicate with a physical device
  TResult initLogicalDevice(const RVkPhysicalDevice& deviceData);
  void destroyLogicalDevice(VkDevice device = nullptr,
                            const VkAllocationCallbacks* pAllocator = nullptr);

  //
  // ***SWAPCHAIN
  //

  // try to initialize swap chain with the desired format and active physical
  // device
  TResult initSwapChain(VkFormat format, VkColorSpaceKHR colorSpace,
                        VkPresentModeKHR presentMode,
                        RVkPhysicalDevice* device = nullptr);

  TResult setSwapChainFormat(const RVkPhysicalDevice& deviceData,
                             const VkFormat& format,
                             const VkColorSpaceKHR& colorSpace);

  TResult setSwapChainPresentMode(const RVkPhysicalDevice& deviceData,
                                  VkPresentModeKHR presentMode);

  TResult setSwapChainExtent(const RVkPhysicalDevice& deviceData);

  TResult setSwapChainImageCount(const RVkPhysicalDevice& deviceData);

  TResult createSwapChainImageTargets();

  // requires valid variables provided by swap chain data gathering methods /
  // initSwapChain
  TResult createSwapChain();
  void destroySwapChain();
  TResult recreateSwapChain();

 private:
  TResult createSwapChainImageViews();

  //
  // ***DEBUG
  //
 public:
  void debug_initialize();

 private:
  void debug_viewMainCamera();
  void debug_viewSunCamera();

  //
  // ***COMPUTE
  //
 private:
  void updateComputeDescriptorSets(RComputeJobInfo* pJobInfo);
  void executeComputeJob(VkCommandBuffer commandBuffer, RComputeJobInfo* pJobInfo);

 public:
  void queueComputeJob(RComputeJobInfo* pInfo);
  void preprocessQueuedComputeJobs(VkCommandBuffer commandBuffer);
  void executeQueuedComputeJobs(VkCommandBuffer commandBuffer);

  //
  // ***RENDERING
  //

 private:
  // Draw bound entities using specific pipeline
  void drawBoundEntitiesIndirect(VkCommandBuffer commandBuffer, EDynamicRenderingPass passOverride = EDynamicRenderingPass::Null);

  void renderEnvironmentMaps(VkCommandBuffer commandBuffer,
                             const uint32_t frameInterval = 1u);

  void prepareFrameResources(VkCommandBuffer commandBuffer);
  void prepareFrameComputeJobs();

  void executeRenderingPass(VkCommandBuffer commandBuffer, EDynamicRenderingPass passId,
                            RMaterial* pPushMaterial = nullptr, bool renderQuad = false);

  void executeShadowPass(VkCommandBuffer commandBuffer, const uint32_t cascadeIndex);

  void executeAOBlurPass(VkCommandBuffer commandBuffer);

  void executePostProcessTAAPass(VkCommandBuffer commandBuffer);
  void executePostProcessSamplingPass(VkCommandBuffer commandBuffer, const uint32_t imageViewIndex, const bool upsample);
  void executePostProcessGetExposurePass(VkCommandBuffer commandBuffer);
  void executePostProcessPass(VkCommandBuffer commandBuffer);

  void executeGUIPass(VkCommandBuffer commandBuffer);

  void executePresentPass(VkCommandBuffer commandBuffer);

 public:
  void renderFrame();
  void renderInitializationFrame();
};

}  // namespace core