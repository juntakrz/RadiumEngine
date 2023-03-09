#pragma once

#include "vk_mem_alloc.h"
#include "core/objects.h"
#include "common.h"
#include "core/world/actors/camera.h"

class WPrimitive;

namespace core {

  class MRenderer {
  private:

    // swapchain data
    struct {
      VkSurfaceFormatKHR formatData;
      VkPresentModeKHR presentMode;
      VkExtent2D imageExtent;
      VkViewport viewport;                                // not used in code, needs to be
      uint32_t imageCount = 0;
      std::vector<VkImage> images;
      std::vector<VkImageView> imageViews;
      std::vector<VkFramebuffer> framebuffers;
    } swapchain;

    // command buffers and pools data
    struct {
      VkCommandPool poolGraphics;
      VkCommandPool poolCompute;
      VkCommandPool poolTransfer;
      std::vector<VkCommandBuffer> buffersGraphics;
      std::vector<VkCommandBuffer> buffersCompute;        // no code for this yet
      std::vector<VkCommandBuffer> buffersTransfer;
    } command;

    // render system data - passes, pipelines, mesh data to render
    struct {
      VkRenderPass renderPass;
      VkPipeline boundPipeline;
      RWorldPipelineSet pipelines;
      uint32_t idIFFrame = 0;                             // in flight frame index
      VkDescriptorPool descriptorPool;
      std::vector<VkDescriptorSet> descriptorSets;
      RDescriptorSetLayouts descriptorSetLayouts;
      std::vector<WPrimitive*> primitives;                // meshes rendered during the current frame
    } system;

    // multi-threaded synchronization objects
    struct {
      std::vector<VkSemaphore> semImgAvailable;
      std::vector<VkSemaphore> semRenderFinished;
      std::vector<VkFence> fenceInFlight;
    } sync;

    // current camera view data
    struct {
      RCameraSettings cameraSettings;

      ACamera* pActiveCamera = nullptr;
      std::vector<RBuffer> modelViewProjectionBuffers;
      RModelViewProjectionUBO modelViewProjectionData;
    } view;

    struct {
      std::vector<RBuffer> buffers;
      RLightingUBO data;
    } lighting;

    struct {
      RImage depth;
    } images;

  public:
    VkInstance APIInstance = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    std::vector<RVkPhysicalDevice> availablePhysicalDevices;
    RVkPhysicalDevice physicalDevice;
    RVkLogicalDevice logicalDevice;
    VmaAllocator memAlloc = nullptr;
    uint32_t frameviewed = 0;
    bool bFramebufferResized = false;

  private:
    MRenderer();

  public:
    static MRenderer& get() {
      static MRenderer _sInstance;
      return _sInstance;
    }
    MRenderer(const MRenderer&) = delete;
    MRenderer& operator=(const MRenderer&) = delete;

    TResult createInstance();
    TResult createInstance(VkApplicationInfo* appInfo);
    TResult destroyInstance();

    TResult initialize();
    void deinitialize();

    // initialize Vulkan memory allocator
    TResult createMemAlloc();
    void destroyMemAlloc();

    // wait until all queues and device are idle
    void waitForSystemIdle();

    // create Vulkan surface in the main window
    TResult createSurface();
    void destroySurface();

    const RDescriptorSetLayouts* getDescriptorSetLayouts() const;
    const VkDescriptorPool getDescriptorPool();

    // returns descriptor set used by the current frame in flight by default
    const VkDescriptorSet getDescriptorSet(uint32_t frameInFlight = -1);

    uint32_t getFrameInFlightIndex();

  private:
    TResult createDescriptorSetLayouts();
    void destroyDescriptorSetLayouts();

    // TODO: improve pool size calculations using loaded map data
    TResult createDescriptorPool();
    void destroyDescriptorPool();

    TResult createDescriptorSets();

    TResult createDepthResources();
    void destroyDepthResources();

    TResult createUniformBuffers();
    void destroyUniformBuffers();
    void updateModelViewProjectionUBOBuffers(uint32_t currentImage);

    VkPipelineShaderStageCreateInfo loadShader(const char* path,
                                               VkShaderStageFlagBits stage);
    VkShaderModule createShaderModule(std::vector<uint8_t>& shaderCode);

    // creates identity MVP matrices
    RModelViewProjectionUBO* getMVPview();

    // creates MVP matrices using currently active camera and model transform
    RModelViewProjectionUBO* updateModelViewProjectionUBO(glm::mat4* pTransform);

    TResult checkInstanceValidationLayers();
    std::vector<const char*> getRequiredInstanceExtensions();
    std::vector<VkExtensionProperties> getInstanceExtensions();

    //
    // MRenderer_util
    //
  private:
    TResult setDepthStencilFormat();

  public:
    /* create buffer for CPU/iGPU or dedicated GPU use:
    defining inData makes the method copy data to an outgoing buffer internally,
    otherwise empty but allocated VkBuffer is the result e.g. for a later data copy.
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
                              uint32_t width, uint32_t height);

    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               ECmdType cmdType = ECmdType::Transfer);

    VkCommandPool getCommandPool(ECmdType type);
    VkQueue getCommandQueue(ECmdType type);

    // creates a command buffer, if 'begin' is true - sets a "one time submit" mode
    VkCommandBuffer createCommandBuffer(ECmdType type,
                                        VkCommandBufferLevel level, bool begin);

    // begins command buffer in a "one time submit" mode
    void beginCommandBuffer(VkCommandBuffer cmdBuffer);

    // end writing to the buffer and submit commands to specific queue,
    // optionally free the buffer and/or use fence
    void flushCommandBuffer(VkCommandBuffer cmdBuffer, ECmdType type,
                            bool free = false, bool useFence = false);

    VkImageView createImageView(VkImage image, VkFormat format,
                                uint32_t levelCount, uint32_t layerCount);

    // binds primitive to graphics pipeline
    uint32_t bindPrimitive(WPrimitive* pPrimitive);

    // binds a vector of primitives and writes binding indices
    void bindPrimitive(const std::vector<WPrimitive*>& inPrimitives,
                       std::vector<uint32_t>& outIndices);

    // unbind primitive by a single index
    void unbindPrimitive(uint32_t index);

    // unbind primitive by the vector of indices
    void unbindPrimitive(const std::vector<uint32_t>& meshIndices);

    // clear all primitive bindings
    void clearPrimitiveBinds();

    // set camera from create cameras by name
    void setCamera(const char* name);
    void setCamera(ACamera* pCamera);

    ACamera* getCamera();

    //
    // MRenderer_physicaldevice
    //
  public:
    // find all available physical devices and store them in graphics manager
    TResult enumPhysicalDevices();

    // automatically use the first valid physical device
    TResult initPhysicalDevice();

    // try to use this physical device if valid
    TResult initPhysicalDevice(const RVkPhysicalDevice& physicalDeviceData);

    // setup physical device database, validate all devices as per usage requirements
    TResult setPhysicalDeviceData(VkPhysicalDevice device,
      RVkPhysicalDevice& outDeviceData);

    // access currently detected devices
    std::vector<RVkPhysicalDevice>& physicalDevices();
    RVkPhysicalDevice* physicalDevices(uint32_t id = 0);

    // provides surface and presentation data for Vulkan swapchain
    TResult queryPhysicalDeviceSwapChainInfo(const RVkPhysicalDevice& deviceData,
      RVkSwapChainInfo& swapChainInfo);

    // find suitable memory type for the required operation on an active physical device
    uint32_t findPhysicalDeviceMemoryType(uint32_t typeFilter,
      VkMemoryPropertyFlags properties);

  private:
    TResult checkPhysicalDeviceExtensionSupport(
      const RVkPhysicalDevice& deviceData);

    std::vector<VkExtensionProperties> getPhysicalDeviceExtensions(
      const RVkPhysicalDevice& deviceData);

    // retrieve queue capabilities for the device, use only first valid indices
    TResult setPhysicalDeviceQueueFamilies(RVkPhysicalDevice& deviceData);

    // -----

    //
    // MRenderer_logicaldevice
    //

  public:
    // creates logical device from the currently active physical one
    TResult initLogicalDevice();

    // create a logical device to communicate with a physical device
    TResult initLogicalDevice(const RVkPhysicalDevice& deviceData);
    void destroyLogicalDevice(VkDevice device = nullptr,
      const VkAllocationCallbacks* pAllocator = nullptr);

    // -----

    //
    // MRenderer_swapchain
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

    // requires valid variables provided by swap chain data gathering methods / initSwapChain
    TResult createSwapChain();
    void destroySwapChain();
    TResult recreateSwapChain();

  private:
    TResult createSwapChainImageViews();
    TResult createFramebuffers();

    // -----

    //
    // MRenderer_rendering
    //

  public:
    TResult createRenderPass();
    void destroyRenderPass();

    TResult createGraphicsPipelines();
    void destroyGraphicsPipelines();
    VkPipelineLayout getWorldPipelineLayout();
    RWorldPipelineSet getWorldPipelineSet();
    VkPipeline getBoundPipeline();

    TResult createCoreCommandPools();
    void destroyCoreCommandPools();

    TResult createCoreCommandBuffers();
    void destroyCoreCommandBuffers();

    TResult recordFrameCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    TResult createSyncObjects();
    void destroySyncObjects();

    TResult drawFrame();

    void updateAspectRatio();
    void setFOV(float FOV);
    void setViewDistance(float farZ);
    void setViewDistance(float nearZ, float farZ);
  };

}