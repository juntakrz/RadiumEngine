#pragma once

#include "vk_mem_alloc.h"
#include "core/objects.h"
#include "common.h"

class WMesh;

class MGraphics {
 private:

  // swapchain data
  struct {
    VkSurfaceFormatKHR formatData;
    VkPresentModeKHR presentMode;
    VkExtent2D imageExtent;
    VkViewport viewport;                      // not used in code, needs to be
    uint32_t imageCount = 0;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
  } gSwapchain;

  // command buffers and pools data
  struct {
    VkCommandPool poolRender;
    VkCommandPool poolTransfer;
    std::vector<VkCommandBuffer> buffersRender;
    std::vector<VkCommandBuffer> buffersTransfer;
  } gCmd;

  // render system data - passes, pipelines, mesh data to render
  struct {
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    uint32_t idIFFrame = 0;                     // in flight frame index
    std::vector<VkDescriptorSetLayout> descSetLayouts;
    std::vector<WMesh*> meshes;                 // meshes rendered during the current frame
  } gSystem;

  // multi-threaded synchronization objects
  struct {
    std::vector<VkSemaphore> sImgAvailable;
    std::vector<VkSemaphore> sRndrFinished;
    std::vector<VkFence> fInFlight;
  } gSync;

  // objects used in shaders
  struct {
    UboMVP uboMVP;                              // matrix*view*projection matrices
    std::vector<RBuffer> buffersUniform;
  } gRender;

  MGraphics();

public:
  VkInstance APIInstance = VK_NULL_HANDLE;
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  std::vector<RVkPhysicalDevice> availablePhysicalDevices;
  RVkPhysicalDevice physicalDevice;
  RVkLogicalDevice logicalDevice;
  VmaAllocator memAlloc;
  uint32_t framesRendered = 0;
  bool bFramebufferResized = false;

 public:
  static MGraphics& get() {
    static MGraphics _sInstance;
    return _sInstance;
  }
  MGraphics(const MGraphics&) = delete;
  MGraphics& operator=(const MGraphics&) = delete;

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

  // binds mesh to graphics pipeline
  uint32_t bindMesh(WMesh* pMesh);

 private:
  TResult createDescriptorSetLayouts();
  void destroyDescriptorSetLayouts();

  TResult createUniformBuffers();
  void destroyUniformBuffers();

  //
  // mgraphics_util
  //
 public:
  /* create buffer for CPU/iGPU or dedicated GPU use:
  * defining inData makes the method copy data to an outgoing buffer internally,
  otherwise empty but allocated VkBuffer is the result e.g. for a later data copy.
  * outAllocInfo is optional, but can be defined if allocation data is required*/
  TResult createBuffer(EBCMode mode, VkDeviceSize size, VkBuffer outBuffer,
                       VmaAllocation outAlloc, void* inData = nullptr,
                       VmaAllocationInfo* outAllocInfo = nullptr);

  // copy buffer with SRC and DST bits
  TResult copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                     VkBufferCopy* copyRegion, uint32_t cmdBufferId = 0);
  TResult copyBuffer(RBuffer* srcBuffer, RBuffer* dstBuffer,
                     VkBufferCopy* copyRegion, uint32_t cmdBufferId = 0);

  //
  // mgraphics_physicaldevice
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
  // mgraphics_logicaldevice
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
  // mgraphics_swapchain
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
  TResult createFramebuffers();
  VkShaderModule createShaderModule(std::vector<char>& shaderCode);

  // -----

  //
  // mgraphics_rendering
  //

 public:
  TResult createRenderPass();
  void destroyRenderPass();

  TResult createGraphicsPipeline();
  void destroyGraphicsPipeline();

  TResult createCommandPools();
  void destroyCommandPools();

  TResult createCommandBuffers();
  void destroyCommandBuffers();
  TResult recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  TResult createSyncObjects();
  void destroySyncObjects();

  TResult drawFrame();

 private:
  TResult checkInstanceValidationLayers();
  std::vector<const char*> getRequiredInstanceExtensions();
  std::vector<VkExtensionProperties> getInstanceExtensions();
};