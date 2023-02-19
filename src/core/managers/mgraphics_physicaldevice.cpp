#include "pch.h"
#include "core/managers/mgraphics.h"
#include "core/renderer/renderer.h"

TResult core::MGraphics::enumPhysicalDevices() {
  uint32_t numDevices = 0, index = 0;
  std::vector<VkPhysicalDevice> physicalDevices;

  vkEnumeratePhysicalDevices(APIInstance, &numDevices, nullptr);

  if (numDevices == 0) {
    RE_LOG(Critical, "Failed to detect any graphics device supporting Vulkan 1.3.");
    return RE_CRITICAL;
  }

  physicalDevices.resize(numDevices);
  availablePhysicalDevices.resize(numDevices);
  vkEnumeratePhysicalDevices(APIInstance, &numDevices, physicalDevices.data());

  for (const auto& device : physicalDevices) {
    setPhysicalDeviceData(device, availablePhysicalDevices.at(index));
    ++index;
  }

  return RE_OK;
}

TResult core::MGraphics::initPhysicalDevice() {
  for (const auto& deviceInfo : availablePhysicalDevices)
    if (initPhysicalDevice(deviceInfo) == RE_OK) return RE_OK;

  RE_LOG(Critical, "No valid physical devices found.");

  return RE_CRITICAL;
}

TResult core::MGraphics::initPhysicalDevice(const RVkPhysicalDevice& device) {
  if (!device.bIsValid) return RE_ERROR;

  physicalDevice = device;

  RE_LOG(Log, "Using physical device: %X %X '%s%'.",
         physicalDevice.properties.vendorID, physicalDevice.properties.deviceID,
         physicalDevice.properties.deviceName);

  return RE_OK;
}

TResult core::MGraphics::setPhysicalDeviceData(VkPhysicalDevice device,
                                         RVkPhysicalDevice& outDeviceData) {
  if (!device) return RE_ERROR;

  TResult chkResult = RE_OK, finalResult = RE_OK;

  outDeviceData.device = device;
  vkGetPhysicalDeviceProperties(device, &outDeviceData.properties);
  vkGetPhysicalDeviceFeatures(device, &outDeviceData.features);
  vkGetPhysicalDeviceMemoryProperties(device, &outDeviceData.memProperties);

  // detect if device supports required queue families
  chkResult = setPhysicalDeviceQueueFamilies(outDeviceData);
  if (chkResult != RE_OK) finalResult = chkResult;

  // detect if device supports required extensions
  chkResult = checkPhysicalDeviceExtensionSupport(outDeviceData);
  if (finalResult == RE_OK && chkResult != RE_OK) finalResult = chkResult;

  // detect if device supports all the required swap chain features
  chkResult = queryPhysicalDeviceSwapChainInfo(outDeviceData,
                                               outDeviceData.swapChainInfo);
  if (finalResult == RE_OK && chkResult != RE_OK) finalResult = chkResult;

  // detect if device is discrete and at least Direct3D11 / OpenGL 4.0 capable
  if (!(outDeviceData.properties.deviceType ==
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
      !outDeviceData.features.tessellationShader &&
      !outDeviceData.features.geometryShader) {
    finalResult = RE_ERROR;
  }

  (finalResult == RE_OK) ? outDeviceData.bIsValid = true
                         : outDeviceData.bIsValid = false;

  return finalResult;
}

std::vector<RVkPhysicalDevice>& core::MGraphics::physicalDevices() {
  return availablePhysicalDevices;
}

RVkPhysicalDevice* core::MGraphics::physicalDevices(uint32_t id) {
  return (id > availablePhysicalDevices.size())
             ? nullptr
             : &availablePhysicalDevices.at(id);
}

TResult core::MGraphics::queryPhysicalDeviceSwapChainInfo(
    const RVkPhysicalDevice& deviceData, RVkSwapChainInfo& outSwapChainInfo) {
  uint32_t formatCount = 0, presentModeCount = 0;
  TResult chkResult = RE_OK;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceData.device, surface,
                                            &outSwapChainInfo.capabilities);

  vkGetPhysicalDeviceSurfaceFormatsKHR(deviceData.device, surface, &formatCount,
                                       nullptr);
  if (!formatCount) chkResult = RE_ERROR;
  outSwapChainInfo.formats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(deviceData.device, surface, &formatCount,
                                       outSwapChainInfo.formats.data());

  vkGetPhysicalDeviceSurfacePresentModesKHR(deviceData.device, surface,
                                            &presentModeCount, nullptr);
  if (!presentModeCount) chkResult = RE_ERROR;
  outSwapChainInfo.modes.resize(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(deviceData.device, surface,
                                            &presentModeCount,
                                            outSwapChainInfo.modes.data());

  return chkResult;
}

uint32_t core::MGraphics::findPhysicalDeviceMemoryType(uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties) {
  for (uint32_t i = 0; i < physicalDevice.memProperties.memoryTypeCount; ++i) {
    if ((typeFilter & (1 << i)) && (physicalDevice.memProperties.memoryTypes[i].propertyFlags &
                                  properties) == properties)
      return i;
  }

  RE_LOG(Critical, "Failed to find suitable memory type on an active physical device.");
  return -1;
}

// private

TResult core::MGraphics::checkPhysicalDeviceExtensionSupport(
    const RVkPhysicalDevice& deviceData) {
  bool bExtSupported = false;
  std::vector<VkExtensionProperties> extProperties =
      getPhysicalDeviceExtensions(deviceData);

  if (extProperties.empty()) {
    return RE_ERROR;
  }

  for (const auto& required : core::renderer::requiredExtensions) {
    bExtSupported = false;

    for (const auto& extension : extProperties)
      if (strcmp(extension.extensionName, required) == 0) {
        bExtSupported = true;
        break;
      }

    if (!bExtSupported) {
      RE_LOG(Warning, "Required extension '%s' is not supported by '%s'.",
             required, deviceData.properties.deviceName);

      return RE_WARNING;
    }
  }

  return RE_OK;
}

std::vector<VkExtensionProperties> core::MGraphics::getPhysicalDeviceExtensions(
    const RVkPhysicalDevice& deviceData) {
  uint32_t extensionCount = 0;
  std::vector<VkExtensionProperties> extProperties;

  vkEnumerateDeviceExtensionProperties(deviceData.device, nullptr,
                                       &extensionCount, nullptr);
  extProperties.resize(extensionCount);
  vkEnumerateDeviceExtensionProperties(deviceData.device, nullptr,
                                       &extensionCount, extProperties.data());

  return extProperties;
}

TResult core::MGraphics::setPhysicalDeviceQueueFamilies(
    RVkPhysicalDevice& deviceData) {
  uint32_t queueFamilyPropertyCount = 0, index = 0;
  VkBool32 presentSupport = false;
  std::vector<VkQueueFamilyProperties> queueFamilyProperties;
  RVkQueueFamilyIndices queueFamilyIndices;

  vkGetPhysicalDeviceQueueFamilyProperties(deviceData.device,
                                           &queueFamilyPropertyCount, nullptr);
  queueFamilyProperties.resize(queueFamilyPropertyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(deviceData.device,
                                           &queueFamilyPropertyCount,
                                           queueFamilyProperties.data());

  // fill queue family indices data structure
  for (const VkQueueFamilyProperties& queueFamilyProperty :
       queueFamilyProperties) {
    if (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT)
      queueFamilyIndices.compute.emplace_back(index);

    if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      queueFamilyIndices.graphics.emplace_back(index);

    vkGetPhysicalDeviceSurfaceSupportKHR(deviceData.device, index, surface,
                                         &presentSupport);
    if (presentSupport) queueFamilyIndices.present.emplace_back(index);

    if ((queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
        queueFamilyIndices.graphics.back() != index &&
        queueFamilyIndices.compute.back() != index &&
        queueFamilyIndices.present.back() != index) {
      queueFamilyIndices.transfer.emplace_back(index);
    }

    presentSupport = false;
    ++index;
  }

  // if transfer queue family isn't found - use a free or first compute family
  if (queueFamilyIndices.transfer.empty()) {
    queueFamilyIndices.transfer.emplace_back(
        (queueFamilyIndices.compute.size() > 1)
            ? queueFamilyIndices.compute.at(1)
            : queueFamilyIndices.compute.at(0));
  }

  deviceData.queueFamilyIndices = queueFamilyIndices;

  if (queueFamilyIndices.graphics.empty() ||
      queueFamilyIndices.compute.empty() ||
      queueFamilyIndices.present.empty()) {
    RE_LOG(Error, "Couldn't retrieve queue families for '%s'.",
           physicalDevice.properties.deviceName);

    return RE_ERROR;
  }

  return RE_OK;
}