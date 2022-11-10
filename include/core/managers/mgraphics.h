#pragma once

#include "vk_mem_alloc.h"
#include "core/objects.h"
#include "common.h"

class WMesh;

class MGraphics {
 private:
  struct {
    VkSurfaceFormatKHR formatData;
    VkPresentModeKHR presentMode;
    VkExtent2D imageExtent;
    uint32_t imageCount = 0;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
  } gSwapchain;

  struct {
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkCommandPool commandPool;
    uint32_t idIFFrame = 0;                     // in flight frame index
    std::vector<VkCommandBuffer> cmdBuffers;
    VkCommandBuffer auxBuffer;                  // single time buffer
    std::vector<WMesh*> meshes;                 // meshes rendered during the current frame
  } gSystem;

  struct {
    std::vector<VkSemaphore> sImgAvailable;
    std::vector<VkSemaphore> sRndrFinished;
    std::vector<VkFence> fInFlight;
  } gSync;

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

  TResult copyBuffer(RBuffer* srcBuffer, RBuffer* dstBuffer,
                     VkBufferCopy* copyRegion);

  //
  // mgraphics_physicaldevice
  //

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

  TResult createCommandPool();
  void destroyCommandPool();

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