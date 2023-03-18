#include "pch.h"
#include "core/core.h"
#include "core/managers/window.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::initSwapChain(VkFormat format, VkColorSpaceKHR colorSpace,
                                 VkPresentModeKHR presentMode,
                                 RVkPhysicalDevice* device) {
  RE_LOG(Log, "Creating the swap chain for presentation.");

  TResult chkResult;

  // use active physical device if none is provided during the method call
  (device) ? device : device = &physicalDevice;

  chkResult = setSwapChainFormat(*device, format, colorSpace);

  // accepting all result values that are not failure
  if (chkResult < RE_CRITICAL) {
    chkResult = setSwapChainPresentMode(*device, presentMode);
  }

  if (chkResult < RE_CRITICAL) {
    chkResult = setSwapChainExtent(*device);
  }

  if (chkResult < RE_CRITICAL) {
    chkResult = setSwapChainImageCount(*device);
  }

  // finally create swap chain used all the data gathered above
  if (chkResult < RE_CRITICAL) {
    chkResult = createSwapChain();
  }

  return chkResult;
}

TResult core::MRenderer::setSwapChainFormat(const RVkPhysicalDevice& deviceData,
                                      const VkFormat& format,
                                      const VkColorSpaceKHR& colorSpace) {
  RE_LOG(Log, "Setting up swap chain format.");

  for (const auto& availableFormat : deviceData.swapChainInfo.formats) {
    if (availableFormat.format == format &&
        availableFormat.colorSpace == colorSpace) {
      swapchain.formatData.format = availableFormat.format;
      swapchain.formatData.colorSpace = availableFormat.colorSpace;

      RE_LOG(Log, "Successfully set the requested format.");
      return RE_OK;
    }
  }

  swapchain.formatData.format = deviceData.swapChainInfo.formats[0].format;
  swapchain.formatData.colorSpace =
      deviceData.swapChainInfo.formats[0].colorSpace;

  RE_LOG(Warning,
         "requested swap chain format was not detected, using the first "
         "available.");
  return RE_WARNING;
}

TResult core::MRenderer::setSwapChainPresentMode(const RVkPhysicalDevice& deviceData,
                                           VkPresentModeKHR presentMode) {
  RE_LOG(Log, "Setting up swap chain present mode.");

  for (const auto& availableMode : deviceData.swapChainInfo.modes) {
    if (availableMode == presentMode) {
      swapchain.presentMode = availableMode;

      RE_LOG(Log, "Successfully set the requested present mode.");
      return RE_OK;
    }
  }

  swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;

  RE_LOG(Warning, 
      "requested present mode was not detected, using "
      "VK_PRESENT_MODE_FIFO_KHR instead.");
  return RE_WARNING;
}

TResult core::MRenderer::setSwapChainExtent(const RVkPhysicalDevice& deviceData) {
  // check if current extent is within limits and set it as active if so
  const VkSurfaceCapabilitiesKHR& capabilities =
      deviceData.swapChainInfo.capabilities;

  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    swapchain.imageExtent = capabilities.currentExtent;
    RE_LOG(Log, "Swap chain extent: %d x %d.", swapchain.imageExtent.width,
           swapchain.imageExtent.height);
    return RE_OK;
  }

  // otherwise determine the correct extent
  int width = 0, height = 0;

  glfwGetFramebufferSize(core::window.getWindow(), &width, &height);
  swapchain.imageExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};
  swapchain.imageExtent.width = std::clamp(
      swapchain.imageExtent.width, capabilities.minImageExtent.width,
      capabilities.maxImageExtent.width);
  swapchain.imageExtent.height = std::clamp(
      swapchain.imageExtent.height, capabilities.minImageExtent.height,
      capabilities.maxImageExtent.height);

  return RE_OK;
}

TResult core::MRenderer::setSwapChainImageCount(const RVkPhysicalDevice& deviceData) {
  const VkSurfaceCapabilitiesKHR& capabilities =
      deviceData.swapChainInfo.capabilities;
  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    imageCount = capabilities.maxImageCount;
  swapchain.imageCount = imageCount;

  return RE_OK;
}

TResult core::MRenderer::createSwapChain() {
  if (!swapchain.imageCount) {
    RE_LOG(Error,
           "swap chain creation failed. Either no capable physical devices "
           "were found or swap chain variables are not valid.");

    return RE_CRITICAL;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.imageFormat = swapchain.formatData.format;
  createInfo.imageColorSpace = swapchain.formatData.colorSpace;
  createInfo.presentMode = swapchain.presentMode;
  createInfo.imageExtent = swapchain.imageExtent;
  createInfo.minImageCount = swapchain.imageCount;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.surface = surface;

  // temporary code to avoid dealing with image ownership
  // images will be shared between graphics/present families
  uint32_t queueFamilyIndices[2] = {
      static_cast<uint32_t>(physicalDevice.queueFamilyIndices.graphics[0]),
      static_cast<uint32_t>(physicalDevice.queueFamilyIndices.present[0])};

  if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform =
      physicalDevice.swapChainInfo.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(logicalDevice.device, &createInfo, nullptr,
                           &swapChain) != VK_SUCCESS) {
    RE_LOG(Critical, "swap chain creation failed during swap chain creation call.");
    return RE_CRITICAL;
  }

  uint32_t swapChainImageCount = 0;
  vkGetSwapchainImagesKHR(logicalDevice.device, swapChain, &swapChainImageCount, nullptr);
  swapchain.images.resize(swapChainImageCount);
  vkGetSwapchainImagesKHR(logicalDevice.device, swapChain, &swapChainImageCount,
                          swapchain.images.data());

  // follow up swap chain resource creation methods
  createSwapChainImageViews();

  return RE_OK;
}

void core::MRenderer::destroySwapChain() {
  RE_LOG(Log, "Cleaning up the swap chain.");

  for (VkFramebuffer framebuffer : swapchain.framebuffers) {
    vkDestroyFramebuffer(logicalDevice.device, framebuffer, nullptr);
  }

  for (VkImageView imageView : swapchain.imageViews) {
    vkDestroyImageView(logicalDevice.device, imageView, nullptr);
  }

  // destroy other framebuffers
  for (auto fb : system.framebuffers) {
    vkDestroyFramebuffer(logicalDevice.device, fb.second, nullptr);
  }

  vkDestroySwapchainKHR(logicalDevice.device, swapChain, nullptr);
}

TResult core::MRenderer::recreateSwapChain() {
  TResult chkResult, finalResult = RE_OK;
  int width, height;
  glfwGetFramebufferSize(core::window.getWindow(), &width, &height);

  if (width == 0 || height == 0) RE_LOG(Log, "Minimization detected.");

  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(core::window.getWindow(), &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(logicalDevice.device);

  destroySwapChain();

  chkResult = createSwapChain();
  if (chkResult = createDepthTarget() > finalResult) finalResult = chkResult;
  if (chkResult = createFramebuffers() > finalResult) finalResult = chkResult;

  return finalResult;
}

// private

TResult core::MRenderer::createSwapChainImageViews() {
  RE_LOG(Log, "Creating image views for swap chain.");

  swapchain.imageViews.resize(swapchain.images.size());

  for (size_t i = 0; i < swapchain.imageViews.size(); ++i) {
    swapchain.imageViews[i] =
        createImageView(swapchain.images[i], swapchain.formatData.format, 1, 1);
  }

  return RE_OK;
}