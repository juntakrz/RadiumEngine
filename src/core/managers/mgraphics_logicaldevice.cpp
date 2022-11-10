#include "pch.h"
#include "core/managers/mgraphics.h"
#include "core/renderer/renderer.h"

TResult MGraphics::initLogicalDevice() { return initLogicalDevice(physicalDevice); }

TResult MGraphics::initLogicalDevice(
    const RVkPhysicalDevice& physicalDeviceData) {
  if (physicalDeviceData.queueFamilyIndices.graphics.empty()) return RE_ERROR;

  std::vector<VkDeviceQueueCreateInfo> deviceQueues;
  float queuePriority = 1.0f;

  std::set<int32_t> queueFamilySet =
      physicalDeviceData.queueFamilyIndices.getAsSet();

  for (const auto& queueFamily : queueFamilySet) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    deviceQueues.emplace_back(queueCreateInfo);
  }

  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = deviceQueues.data();
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(deviceQueues.size());
  deviceCreateInfo.pEnabledFeatures = &physicalDeviceData.features;
  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(core::renderer::requiredExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames =
      core::renderer::requiredExtensions.data();
  deviceCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(requiredLayers.size());
  deviceCreateInfo.ppEnabledLayerNames = requiredLayers.data();

  if (bRequireValidationLayers) {
    deviceCreateInfo.enabledLayerCount =
        static_cast<uint32_t>(debug::validationLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = debug::validationLayers.data();
  }

  if (vkCreateDevice(physicalDeviceData.device, &deviceCreateInfo, nullptr,
                     &logicalDevice.device) != VK_SUCCESS) {
    RE_LOG(Error,
           "failed to create logical device using "
           "provided physical device: '%s'.",
           physicalDeviceData.properties.deviceName);
    return RE_ERROR;
  }

  // create graphics queue for a logical device
  vkGetDeviceQueue(logicalDevice.device,
                   physicalDevice.queueFamilyIndices.graphics[0], 0,
                   &logicalDevice.queues.graphics);

  // create compute queue for a logical device
  vkGetDeviceQueue(logicalDevice.device,
                   physicalDevice.queueFamilyIndices.compute[0], 0,
                   &logicalDevice.queues.compute);

  // create present queue for a logical device
  vkGetDeviceQueue(logicalDevice.device,
                   physicalDevice.queueFamilyIndices.present[0], 0,
                   &logicalDevice.queues.present);

  // create transfer queue for a logical device
  vkGetDeviceQueue(logicalDevice.device,
                   physicalDevice.queueFamilyIndices.transfer[0], 0,
                   &logicalDevice.queues.transfer);

  RE_LOG(Log,
         "Successfully created logical device for '%s', handle: 0x%016llX.",
         physicalDeviceData.properties.deviceName, physicalDeviceData.device);
  return RE_OK;
}

void MGraphics::destroyLogicalDevice(VkDevice device,
                                     const VkAllocationCallbacks* pAllocator) {
  if (!device) device = logicalDevice.device;
  RE_LOG(Log, "Destroying logical device, handle: 0x%016llX.", device);
  vkFreeMemory(logicalDevice.device, logicalDevice.vertexBufferMemory, nullptr);
  vkDestroyDevice(device, pAllocator);
}