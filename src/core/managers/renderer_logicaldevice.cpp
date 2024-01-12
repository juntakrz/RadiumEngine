#include "pch.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::initLogicalDevice() { return initLogicalDevice(physicalDevice); }

TResult core::MRenderer::initLogicalDevice(
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

  // Vulkan 1.3: Enabling descriptor indexing
  VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures{};
  descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
  descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
  descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
  descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
  descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
  descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
  descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

  // Vulkan 1.3: Enabling buffer device address
  VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeatures{};
  bdaFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
  bdaFeatures.bufferDeviceAddress = VK_TRUE;
  bdaFeatures.pNext = &descriptorIndexingFeatures;

  // Vulkan 1.3: Enabling dynamic rendering
  VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
  dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
  dynamicRenderingFeatures.pNext = &bdaFeatures;

  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = deviceQueues.data();
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(deviceQueues.size());
  deviceCreateInfo.pEnabledFeatures = &physicalDeviceData.deviceFeatures.features;
  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(core::vulkan::requiredExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames =
      core::vulkan::requiredExtensions.data();
  deviceCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(requiredLayers.size());
  deviceCreateInfo.ppEnabledLayerNames = requiredLayers.data();

  //Vulkan 1.3: Enabling dynamic rendering
  deviceCreateInfo.pNext = &dynamicRenderingFeatures;

  if (requireValidationLayers) {
    deviceCreateInfo.enabledLayerCount =
        static_cast<uint32_t>(debug::validationLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = debug::validationLayers.data();
  }

  if (vkCreateDevice(physicalDeviceData.device, &deviceCreateInfo, nullptr,
                     &logicalDevice.device) != VK_SUCCESS) {
    RE_LOG(Error,
           "failed to create logical device using "
           "provided physical device: '%s'.",
           physicalDeviceData.deviceProperties.properties.deviceName);
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
         physicalDeviceData.deviceProperties.properties.deviceName, physicalDeviceData.device);
  return RE_OK;
}

void core::MRenderer::destroyLogicalDevice(VkDevice device,
                                     const VkAllocationCallbacks* pAllocator) {
  if (!device) device = logicalDevice.device;
  RE_LOG(Log, "Destroying logical device, handle: 0x%016llX.", device);
  vkDestroyDevice(device, pAllocator);
}