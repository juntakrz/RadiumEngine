#pragma once

#include "vk_mem_alloc.h"
#include "core/objects.h"
#include "core/async.h"
#include "common.h"
#include "core/world/actors/camera.h"

class WPrimitive;
struct RTexture;

namespace core {

class MRenderer {
 private:
  // command buffers and pools data
  struct {
    VkCommandPool poolGraphics;
    VkCommandPool poolCompute;
    VkCommandPool poolTransfer;
    std::vector<VkCommandBuffer> buffersGraphics;
    std::vector<VkCommandBuffer> buffersCompute;  // TODO: not implemented yet
    std::vector<VkCommandBuffer> buffersTransfer;
  } command;

  struct {
    VkDescriptorSet imageDescriptorSet;
    VkExtent3D imageExtent;
    RComputeImagePCB imagePCB;

    std::vector<RComputeJobInfo> jobs;
  } compute;

  struct REnvironmentData {
    REnvironmentFragmentPCB pushBlock;
    VkDescriptorSet LUTDescriptorSet;
    VkImageSubresourceRange subresourceRange;
    int32_t genInterval = 2;
    RTexture* pTargetCubemap = nullptr;
    std::array<glm::vec3, 6> cameraTransformVectors;

    struct {
      RComputeJobInfo LUT;
      RComputeJobInfo irradiance;
      RComputeJobInfo prefiltered;
    } computeJobs;

    struct {
      uint32_t layer = 0;     // cubemap layer
    } tracking;
  } environment;

  struct {
    std::vector<RBuffer> buffers;
    RLightingUBO data;

    struct {
      int8_t bufferUpdatesRemaining = 0;
      bool dataRequiresUpdate = false;
    } tracking;
  } lighting;

  struct RMaterialBuffers {
    VkDescriptorSet descriptorSet;
  } material;

  struct RSceneBuffers {
    RBuffer vertexBuffer;
    RBuffer indexBuffer;
    RBuffer rootTransformBuffer;
    RBuffer nodeTransformBuffer;
    RBuffer skinTransformBuffer;
    uint32_t currentVertexOffset = 0u;
    uint32_t currentIndexOffset = 0u;
    VkDescriptorSet transformDescriptorSet;

    std::vector<RTexture*> pGBufferTargets;
    VkDescriptorSet GBufferDescriptorSet;

    // per frame in flight buffered camera/lighting descriptor sets
    std::vector<VkDescriptorSet> descriptorSets;
  } scene;

  // swapchain data
  struct {
    VkSurfaceFormatKHR formatData;
    VkPresentModeKHR presentMode;
    VkExtent2D imageExtent;
    uint32_t imageCount = 0;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<RFramebuffer> framebuffers;
    VkImageCopy copyRegion;
  } swapchain;

  // multi-threaded synchronization objects
  struct {
    std::vector<VkSemaphore> semImgAvailable;
    std::vector<VkSemaphore> semRenderFinished;
    std::vector<VkFence> fenceInFlight;
    RAsync asyncUpdateEntities;
  } sync;

  // render system data - passes, pipelines, mesh data to render
  struct {
    std::unordered_map<EDynamicRenderingPass, RDynamicRenderingPass> dynamicRenderingPasses;
    std::unordered_map<ERenderPass, RRenderPass> renderPasses;
    std::unordered_map<EPipelineLayout, VkPipelineLayout> layouts;
    std::unordered_map<EPipeline, RPipeline> graphicsPipelines;
    std::unordered_map<EComputePipeline, VkPipeline> computePipelines;
    std::unordered_map<std::string, RFramebuffer> framebuffers;  // general purpose, swapchain uses its own set
    std::vector<RViewport> viewports;

    VkDescriptorPool descriptorPool;
    std::unordered_map<EDescriptorSetLayout, VkDescriptorSetLayout> descriptorSetLayouts;

    std::vector<REntityBindInfo> bindings;  // entities rendered during the current frame
    std::vector<VkDrawIndexedIndirectCommand> drawCommands;
    std::unordered_map<EPipeline, std::vector<WPrimitive*>> primitivesByPipeline;  // TODO

    VkRenderPassBeginInfo renderPassBeginInfo;
  } system;

  // current camera view data
  struct {
    RCameraInfo cameraSettings;
    ACamera* pActiveCamera = nullptr;
    ACamera* pSunCamera = nullptr;
    std::vector<RBuffer> modelViewProjectionBuffers;
    RSceneUBO worldViewProjectionData;
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
    void* pCurrentMesh = nullptr;
    void* pCurrentMaterial = nullptr;

    RRenderPass* pCurrentRenderPass = nullptr;
    RPipeline* pCurrentPipeline = nullptr;
    VkPipelineLayout currentPipelineLayout = VK_NULL_HANDLE;
    uint32_t currentFrameIndex = 0;
    uint32_t frameInFlight = 0;
    uint32_t framesRendered = 0;
    bool generateEnvironmentMapsImmediate = false;  // queue single pass environment map gen (slow)
    bool generateEnvironmentMaps = false;           // queue sequenced environment map gen (fast)
    bool isEnvironmentPass = false;                 // is in the process of generating

    void refresh() {
      pCurrentMesh = nullptr;
      pCurrentMaterial = nullptr;
      //pCurrentPipeline = nullptr;
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

  TResult createUniformBuffers();
  void destroyUniformBuffers();

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

  VkDescriptorSet getMaterialDescriptorSet();
  RMaterialBuffers* getMaterialBuffers();
  RSceneBuffers* getSceneBuffers();
  RSceneUBO* getSceneUBO();
  REnvironmentData* getEnvironmentData();

  void queueLightingUBOUpdate();
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

  TResult createDefaultFramebuffers();

  TResult createPipelineLayouts();
  TResult createGraphicsPipelines();
  void destroyGraphicsPipelines();
  TResult createComputePipelines();
  void destroyComputePipelines();
  VkPipelineLayout& getPipelineLayout(EPipelineLayout type);
  TResult createGraphicsPipeline(RGraphicsPipelineInfo* pipelineInfo);
  RPipeline* getGraphicsPipeline(EPipeline type);
  VkPipeline& getComputePipeline(EComputePipeline type);

  // check if pipeline flag is present in the flag array
  bool checkPipeline(uint32_t pipelineFlags, EPipeline pipelineFlag);

  //
  // ***PASS
  //

  /* Will create a multi subpass render pass for selected types and will expect a type-dependent color attachment layout */
  TResult createRenderPass(ERenderPass renderPassId, EPipeline pipeline, RRenderPassInfo* pInfo);

  // Create new dynamic rendering pass and/or add new/update existing attached pipeline
  TResult createDynamicRenderPass(EDynamicRenderingPass passId, RDynamicRenderingInfo* pInfo);

  TResult createRenderPasses();
  void destroyRenderPasses();
  RRenderPass* getRenderPass(ERenderPass type);
  VkRenderPass& getVkRenderPass(ERenderPass type);

  TResult createDynamicRenderingPasses();
  RDynamicRenderingPass* getDynamicRenderingPass(EDynamicRenderingPass type);

  TResult configureRenderPasses();

  //
  // ***UTIL
  //

 private:
  TResult createFramebuffer(ERenderPass renderPass,
                            const std::vector<std::string>& attachmentNames,
                            const char* framebufferName);

  // create single layer render target for fragment shader output
  // uses swapchain resolution
  RTexture* createFragmentRenderTarget(const char* name, uint32_t width = 0,
                                       uint32_t height = 0);

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

public:
  TResult copyImage(VkCommandBuffer cmdBuffer, VkImage srcImage,
                    VkImage dstImage, VkImageLayout srcImageLayout,
                    VkImageLayout dstImageLayout, VkImageCopy& copyRegion);

  void setImageLayout(VkCommandBuffer cmdBuffer, RTexture* pTexture,
                      VkImageLayout newLayout,
                      VkImageSubresourceRange subresourceRange);

  void setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                      VkImageLayout oldLayout, VkImageLayout newLayout,
                      VkImageSubresourceRange subresourceRange);

  void convertRenderTargets(VkCommandBuffer cmdBuffer,
                            std::vector<RTexture*>* pInTextures,
                            bool convertBackToRenderTargets);

  TResult generateMipMaps(RTexture* pTexture, int32_t mipLevels,
                          VkFilter filter = VK_FILTER_LINEAR);

  // generates a single mip map from a previous level at a set layer
  TResult generateSingleMipMap(VkCommandBuffer cmdBuffer, RTexture* pTexture,
                               uint32_t mipLevel, uint32_t layer = 0,
                               VkFilter filter = VK_FILTER_LINEAR);

  VkCommandPool getCommandPool(ECmdType type);
  VkQueue getCommandQueue(ECmdType type);

  // creates a command buffer, if 'begin' is true - sets a "one time submit"
  // mode
  VkCommandBuffer createCommandBuffer(ECmdType type, VkCommandBufferLevel level,
                                      bool begin);

  // begins command buffer in a "one time submit" mode
  void beginCommandBuffer(VkCommandBuffer cmdBuffer);

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

  // set camera from create cameras by name
  void setCamera(const char* name);
  void setCamera(ACamera* pCamera);

  void setSunCamera(const char* name);
  void setSunCamera(ACamera* pCamera);

  // get current renderer camera
  ACamera* getCamera();

  void setIBLScale(float newScale);

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
   TResult copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage,
     uint32_t width, uint32_t height,
     uint32_t layerCount);

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
  // Add images to be processed to compute descriptor set, can be added at offsets
  void updateComputeImageSet(std::vector<RTexture*>* pInImages, std::vector<RTexture*>* pInSamplers = nullptr,
    const bool useExtraImageViews = false, const bool useExtraSamplerViews = false);
  void executeComputeImage(VkCommandBuffer commandBuffer,
     EComputePipeline pipeline);

  void generateLUTMap();

 public:
  void createComputeJob(RComputeJobInfo* pInfo);
  void executeComputeJobs();

  //
  // ***RENDERING
  //

 private:
  void updateBoundEntities();

  // draw bound entities using render pass pipelines
  void drawBoundEntities(VkCommandBuffer cmdBuffer, uint32_t subpassIndex = 0);

  // draw bound entities using specific pipeline
  void drawBoundEntities(VkCommandBuffer, EPipeline forcedPipeline);

  void renderPrimitive(VkCommandBuffer cmdBuffer, WPrimitive* pPrimitive,
                       EPipeline pipelineFlag, REntityBindInfo* pBindInfo);

  void renderEnvironmentMaps(VkCommandBuffer commandBuffer,
                             const uint32_t frameInterval = 1);

  void executeRenderPass(VkCommandBuffer commandBuffer, ERenderPass passId,
                         VkDescriptorSet* pSceneSets, const uint32_t setCount);

  void executeDynamicRendering(VkCommandBuffer commandBuffer,
                               EDynamicRenderingPass renderPass);

  // Pipeline must use a compatible quad drawing vertex shader
  // Scene descriptor set is optional and is required only if fragment shader needs scene data
  void renderFullscreenQuad(VkCommandBuffer commandBuffer,
                            EPipelineLayout pipelineLayout, EPipeline pipeline,
                            VkDescriptorSet* pAttachmentSet,
                            VkDescriptorSet* pSceneSet = nullptr,
                            uint32_t sceneDynamicOffset = 0u);

 public:
  void renderFrame();
  void renderInitFrame();
};

}  // namespace core