#include "pch.h"
#include "core/managers/mgraphics.h"
#include "core/managers/mwindow.h"

TResult MGraphics::initSwapChain(VkFormat format, VkColorSpaceKHR colorSpace,
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

TResult MGraphics::setSwapChainFormat(const RVkPhysicalDevice& deviceData,
                                      const VkFormat& format,
                                      const VkColorSpaceKHR& colorSpace) {
  RE_LOG(Log, "Setting up swap chain format.");

  for (const auto& availableFormat : deviceData.swapChainInfo.formats) {
    if (availableFormat.format == format &&
        availableFormat.colorSpace == colorSpace) {
      dataSwapChain.formatData.format = availableFormat.format;
      dataSwapChain.formatData.colorSpace = availableFormat.colorSpace;

      RE_LOG(Log, "Successfully set the requested format.");
      return RE_OK;
    }
  }

  dataSwapChain.formatData.format = deviceData.swapChainInfo.formats[0].format;
  dataSwapChain.formatData.colorSpace =
      deviceData.swapChainInfo.formats[0].colorSpace;

  RE_LOG(Warning,
         "requested swap chain format was not detected, using the first "
         "available.");
  return RE_WARNING;
}

TResult MGraphics::setSwapChainPresentMode(const RVkPhysicalDevice& deviceData,
                                           VkPresentModeKHR presentMode) {
  RE_LOG(Log, "Setting up swap chain present mode.");

  for (const auto& availableMode : deviceData.swapChainInfo.modes) {
    if (availableMode == presentMode) {
      dataSwapChain.presentMode = availableMode;

      RE_LOG(Log, "Successfully set the requested present mode.");
      return RE_OK;
    }
  }

  dataSwapChain.presentMode = VK_PRESENT_MODE_FIFO_KHR;

  RE_LOG(Warning, 
      "requested present mode was not detected, using "
      "VK_PRESENT_MODE_FIFO_KHR instead.");
  return RE_WARNING;
}

TResult MGraphics::setSwapChainExtent(const RVkPhysicalDevice& deviceData) {
  // check if current extent is within limits and set it as active if so
  const VkSurfaceCapabilitiesKHR& capabilities =
      deviceData.swapChainInfo.capabilities;

  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    dataSwapChain.imageExtent = capabilities.currentExtent;
    RE_LOG(Log, "Swap chain extent: %d x %d.", dataSwapChain.imageExtent.width,
           dataSwapChain.imageExtent.height);
    return RE_OK;
  }

  // otherwise determine the correct extent
  int width = 0, height = 0;

  glfwGetFramebufferSize(MWindow::get().window(), &width, &height);
  dataSwapChain.imageExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};
  dataSwapChain.imageExtent.width = std::clamp(
      dataSwapChain.imageExtent.width, capabilities.minImageExtent.width,
      capabilities.maxImageExtent.width);
  dataSwapChain.imageExtent.height = std::clamp(
      dataSwapChain.imageExtent.height, capabilities.minImageExtent.height,
      capabilities.maxImageExtent.height);

  return RE_OK;
}

TResult MGraphics::setSwapChainImageCount(const RVkPhysicalDevice& deviceData) {
  const VkSurfaceCapabilitiesKHR& capabilities =
      deviceData.swapChainInfo.capabilities;
  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    imageCount = capabilities.maxImageCount;
  dataSwapChain.imageCount = imageCount;

  return RE_OK;
}

TResult MGraphics::createSwapChain() {
  if (!dataSwapChain.imageCount) {
    RE_LOG(Error,
           "swap chain creation failed. Either no capable physical devices "
           "were found or swap chain variables are not valid.");

    return RE_CRITICAL;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.imageFormat = dataSwapChain.formatData.format;
  createInfo.imageColorSpace = dataSwapChain.formatData.colorSpace;
  createInfo.presentMode = dataSwapChain.presentMode;
  createInfo.imageExtent = dataSwapChain.imageExtent;
  createInfo.minImageCount = dataSwapChain.imageCount;
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
  dataSwapChain.images.resize(swapChainImageCount);
  vkGetSwapchainImagesKHR(logicalDevice.device, swapChain, &swapChainImageCount,
                          dataSwapChain.images.data());

  // follow up swap chain resource creation methods
  createSwapChainImageViews();

  return RE_OK;
}

void MGraphics::destroySwapChain() {
  RE_LOG(Log, "Cleaning up the swap chain.");

  for (VkFramebuffer framebuffer : dataSwapChain.framebuffers) {
    vkDestroyFramebuffer(logicalDevice.device, framebuffer, nullptr);
  }

  for (VkImageView imageView : dataSwapChain.imageViews) {
    vkDestroyImageView(logicalDevice.device, imageView, nullptr);
  }

  vkDestroySwapchainKHR(logicalDevice.device, swapChain, nullptr);
}

TResult MGraphics::recreateSwapChain() {
  TResult chkResult, finalResult = RE_OK;
  int width, height;
  glfwGetFramebufferSize(MWindow::get().window(), &width, &height);

  if (width == 0 || height == 0) RE_LOG(Log, "Minimization detected.");

  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(MWindow::get().window(), &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(logicalDevice.device);

  destroySwapChain();

  chkResult = createSwapChain();
  if (chkResult = createFramebuffers() > finalResult) finalResult = chkResult;

  return finalResult;
}

// private

TResult MGraphics::createSwapChainImageViews() {
  RE_LOG(Log, "Creating image views for swap chain.");

  dataSwapChain.imageViews.resize(dataSwapChain.images.size());

  for (size_t i = 0; i < dataSwapChain.imageViews.size(); ++i) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = dataSwapChain.images[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = dataSwapChain.formatData.format;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;

    if (vkCreateImageView(logicalDevice.device, &viewInfo, nullptr,
                          &dataSwapChain.imageViews[i]) != VK_SUCCESS) {
      RE_LOG(Error, "failed to create swapchain image view %d.", i);

      return RE_ERROR;
    }
  }

  return RE_OK;
}

TResult MGraphics::createFramebuffers() {
  RE_LOG(Log, "Creating framebuffers.");

  dataSwapChain.framebuffers.resize(dataSwapChain.imageViews.size());

  for (size_t i = 0; i < dataSwapChain.framebuffers.size(); ++i) {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = dataSystem.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &dataSwapChain.imageViews[i];
    framebufferInfo.width = dataSwapChain.imageExtent.width;
    framebufferInfo.height = dataSwapChain.imageExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(logicalDevice.device, &framebufferInfo, nullptr,
                            &dataSwapChain.framebuffers[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "failed to create frame buffer with index %d.", i);

      return RE_CRITICAL;
    }
  }

  return RE_OK;
}