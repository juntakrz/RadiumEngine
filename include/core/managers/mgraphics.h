#pragma once

#include "core/objects.h"
#include "common.h"

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
  } dataSwapChain;

  struct {
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> cmdBuffers;
    uint32_t idFrame = 0;                       // in flight frame index
    struct {
      std::vector<VkSemaphore> sImgAvailable;
      std::vector<VkSemaphore> sRndrFinished;
      std::vector<VkFence> fInFlight;
    } sync;
  } dataRender;

MGraphics();

public:
  VkInstance APIInstance = VK_NULL_HANDLE;
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  std::vector<RVkPhysicalDevice> availablePhysicalDevices;
  RVkPhysicalDevice physicalDevice;
  RVkLogicalDevice logicalDevice;
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

  // wait until all queues and device are idle
  void waitForSystemIdle();

  //
  // mgraphics_physicaldevice
  //

  // generic initialization method that uses physical device methods to set
  // default physical device
  TResult initDefaultPhysicalDevice();

  // store all physical devices available in the graphics manager
  TResult detectPhysicalDevices();

  // try to use this physical device if valid
  TResult usePhysicalDevice(const RVkPhysicalDevice& physicalDeviceData);

  // use the first valid physical device
  TResult useFirstValidPhysicalDevice();

  // fill in physical device information structure, also performs device
  // validity checks
  TResult setPhysicalDeviceData(VkPhysicalDevice device,
                                RVkPhysicalDevice& outDeviceData);

  // access currently detected devices
  std::vector<RVkPhysicalDevice>& physicalDevices();
  RVkPhysicalDevice* physicalDevices(uint32_t id = 0);

  // provides surface and presentation data for Vulkan swapchain
  TResult queryPhysicalDeviceSwapChainInfo(const RVkPhysicalDevice& deviceData,
                                           RVkSwapChainInfo& swapChainInfo);

 private:
  TResult checkPhysicalDeviceExtensionSupport(
      const RVkPhysicalDevice& deviceData);

  std::vector<VkExtensionProperties> getPhysicalDeviceExtensions(
      const RVkPhysicalDevice& deviceData);

  // retrieve queue capabilities for the device, use only first valid indices
  TResult setPhysicalDeviceQueueFamilies(RVkPhysicalDevice& deviceData);

  // -----

 public:
  // create a logical device to communicate with a physical device
  TResult createLogicalDevice(const RVkPhysicalDevice& deviceData);
  void destroyLogicalDevice(VkDevice device = nullptr,
                            const VkAllocationCallbacks* pAllocator = nullptr);

  // create Vulkan surface in the main window
  TResult createSurface();
  void destroySurface();

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
  TResult recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  TResult createSyncObjects();
  void destroySyncObjects();

  TResult drawFrame();

 private:
  TResult checkInstanceValidationLayers();
  std::vector<const char*> getRequiredInstanceExtensions();
  std::vector<VkExtensionProperties> getInstanceExtensions();
};