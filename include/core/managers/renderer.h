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

  struct REnvironmentData {
    REnvironmentPCB envPushBlock;
    std::vector<VkDescriptorSet> envDescriptorSets;
    VkDescriptorSet LUTDescriptorSet;
    std::vector<RBuffer> transformBuffers;
    VkDeviceSize transformOffset = 0u;
    RTexture* pSourceTexture;
    VkImageSubresourceRange sourceRange;
    std::vector<RTexture*> destinationTextures;
    std::vector<VkImageSubresourceRange> destinationRanges;
    VkImageCopy copyRegion;
    int32_t genInterval = 3;

    struct {
      uint32_t pipeline = 0; // ENVFILTER or ENVIRRAD
      uint32_t layer = 0;    // cubemap layer
      uint32_t mipLevel = 0; // mipmap level
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

  struct RSceneBuffers {
    RBuffer vertexBuffer;
    RBuffer indexBuffer;
    RBuffer rootTransformBuffer;
    RBuffer nodeTransformBuffer;
    RBuffer skinTransformBuffer;
    uint32_t currentVertexOffset = 0u;
    uint32_t currentIndexOffset = 0u;
    VkDescriptorSet transformSet;
  } scene;

  // swapchain data
  struct {
    VkSurfaceFormatKHR formatData;
    VkPresentModeKHR presentMode;
    VkExtent2D imageExtent;
    uint32_t imageCount = 0;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
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
    std::unordered_map<EPipelineLayout, VkPipelineLayout> layouts;
    std::unordered_map<EPipeline, VkPipeline> pipelines;
    std::unordered_map<ERenderPass, RRenderPass> renderPasses;
    VkRenderPassBeginInfo renderPassBeginInfo;
    std::unordered_map<std::string, VkFramebuffer> framebuffers;   // general purpose, swapchain uses its own set

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::unordered_map<EDescriptorSetLayout, VkDescriptorSetLayout>
        descriptorSetLayouts;

    std::vector<REntityBindInfo> bindings;                         // entities rendered during the current frame
    std::vector<VkDrawIndexedIndirectCommand> drawCommands;
    std::unordered_map<EPipeline, std::vector<WPrimitive*>> primitivesByPipeline;   // TODO
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
    void* pCurrentPipeline = nullptr;

    RRenderPass* pCurrentRenderPass = nullptr;
    uint32_t frameInFlight = 0;
    uint32_t framesRendered = 0;
    bool generateEnvironmentMapsImmediate = false;  // queue single pass environment map gen (slow)
    bool generateEnvironmentMaps = false;           // queue sequenced environment map gen (fast)
    bool isEnvironmentPass = false;                 // is in the process of generating

    void refresh() {
      pCurrentMesh = nullptr;
      pCurrentMaterial = nullptr;
      pCurrentPipeline = nullptr;
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

  // TODO: improve pool size calculations using loaded map data
  TResult createDescriptorPool();
  void destroyDescriptorPool();

  TResult createUniformBuffers();
  void destroyUniformBuffers();

  TResult createImageTargets();
  TResult createGBufferRenderTargets();
  TResult createDepthTargets();
  TResult createRendererDefaults();

  TResult createCoreCommandPools();
  void destroyCoreCommandPools();

  TResult createCoreCommandBuffers();
  void destroyCoreCommandBuffers();

  TResult createSyncObjects();
  void destroySyncObjects();

  void setEnvironmentUBO();
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

  const VkDescriptorSetLayout getDescriptorSetLayout(EDescriptorSetLayout type) const;
  const VkDescriptorPool getDescriptorPool();

  // returns descriptor set used by the current frame in flight by default
  const VkDescriptorSet getDescriptorSet(uint32_t frameInFlight = -1);

  RSceneBuffers* getSceneBuffers();

  RSceneUBO* getSceneUBO();

  void queueLightingUBOUpdate();
  void updateLightingUBO(const int32_t frameIndex);

  //
  // ***PIPELINE
  //

  TResult createDescriptorSetLayouts();
  void destroyDescriptorSetLayouts();

  TResult createDescriptorSets();

  TResult createDefaultFramebuffers();

  TResult createRenderPasses();
  void destroyRenderPasses();
  RRenderPass* getRenderPass(ERenderPass type);
  VkRenderPass& getVkRenderPass(ERenderPass type);

  TResult createGraphicsPipelines();
  void destroyGraphicsPipelines();
  VkPipelineLayout& getPipelineLayout(EPipelineLayout type);
  VkPipeline& getPipeline(EPipeline type);

  // check if pipeline flag is present in the flag array
  bool checkPipeline(uint32_t pipelineFlags, EPipeline pipelineFlag);

  TResult configureRenderPasses();

  //
  // ***UTIL
  //

 private:
  VkRenderPass createRenderPass(VkDevice device, uint32_t colorAttachments,
                                VkAttachmentDescription* pColorAttachments,
                                VkAttachmentDescription* pDepthAttachment);

  TResult createFramebuffer(ERenderPass renderPass,
                            const std::vector<std::string>& attachmentNames,
                            const char* framebufferName);

  // create single layer render target for fragment shader output
  // uses swapchain resolution
  RTexture* createFragmentRenderTarget(const char* name);

  void setResourceName(VkDevice device, VkObjectType objectType,
                       uint64_t handle, const char* name);

  TResult getDepthStencilFormat(VkFormat desiredFormat, VkFormat& outFormat);

  VkPipelineShaderStageCreateInfo loadShader(const char* path,
                                             VkShaderStageFlagBits stage);
  VkShaderModule createShaderModule(std::vector<char>& shaderCode);

  TResult checkInstanceValidationLayers();
  std::vector<const char*> getRequiredInstanceExtensions();
  std::vector<VkExtensionProperties> getInstanceExtensions();

 public:
  /* create buffer for CPU/iGPU or dedicated GPU use:
  defining inData makes the method copy data to an outgoing buffer internally,
  otherwise empty but allocated VkBuffer is the result e.g. for a later data
  copy.
  */
  TResult createBuffer(EBufferMode mode, VkDeviceSize size, RBuffer& outBuffer,
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

  VkImageView createImageView(VkImage image, VkFormat format,
                              uint32_t levelCount, uint32_t layerCount);

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
  // ***PHYSICAL DEVICE
  //

 public:
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
  // ***RENDERING
  //

 private:
  void updateBoundEntities();

  // draw bound entities using render pass pipelines
  void drawBoundEntities(VkCommandBuffer cmdBuffer);

  // draw bound entities using specific pipeline
  void drawBoundEntities(VkCommandBuffer, EPipeline forcedPipeline);

  void renderPrimitive(VkCommandBuffer cmdBuffer, WPrimitive* pPrimitive,
                       EPipeline pipelineFlag, REntityBindInfo* pBindInfo);

  // DEPRECATED - generates all environment maps and mipmaps in a single pass
  void renderEnvironmentMaps(VkCommandBuffer commandBuffer);

  void renderEnvironmentMapsSequenced(VkCommandBuffer commandBuffer,
                                      int32_t frameInterval = 1);

  // generates BRDF LUT map
  void generateLUTMap();

 public:
  void executeRenderPass(VkCommandBuffer commandBuffer, ERenderPass passType,
                         VkDescriptorSet* pFrameSets, const uint32_t setCount,
                         VkFramebuffer framebuffer);

  void renderFrame();
  void renderInitFrame();

  void updateAspectRatio();
  void setFOV(float FOV);
  void setViewDistance(float farZ);
  void setViewDistance(float nearZ, float farZ);
};

}  // namespace core