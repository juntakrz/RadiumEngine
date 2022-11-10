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

  // create present queue for a logical device
  vkGetDeviceQueue(logicalDevice.device,
                   physicalDevice.queueFamilyIndices.present[0], 0,
                   &logicalDevice.queues.present);

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

TResult MGraphics::createBuffer(VkDeviceSize size,
                                             VkBufferUsageFlags usage,
                                             RBuffer* outBuffer) {
  /* if (!outBuffer) {
    RE_LOG(Error, "No outgoing buffer was provided.");
    return RE_ERROR;
  }

  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCreateInfo.usage = usage;
  bufferCreateInfo.size = size;

  if (vkCreateBuffer(mgrGfx->logicalDevice.device, &bufferCreateInfo, nullptr,
                     &outBuffer->buffer) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create buffer.");
    return RE_ERROR;
  };

  vkGetBufferMemoryRequirements(logicalDevice.device, outBuffer->buffer,
                                &outBuffer->memRequirements);

  outBuffer->size = size;*/

  return RE_OK;
}

TResult MGraphics::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                RBuffer* outBuffer, VkDeviceMemory& outMemory) {
  /* if (!outBuffer) {
    RE_LOG(Error, "No outgoing buffer was provided.");
    return RE_ERROR;
  }

  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCreateInfo.usage = usage;
  bufferCreateInfo.size = size;

  if (vkCreateBuffer(mgrGfx->logicalDevice.device, &bufferCreateInfo, nullptr,
                     &outBuffer->buffer) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create buffer.");
    return RE_ERROR;
  };

  vkGetBufferMemoryRequirements(logicalDevice.device, outBuffer->buffer,
                                &outBuffer->memRequirements);

  outBuffer->size = size;

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = outBuffer->memRequirements.size;
  allocInfo.memoryTypeIndex = mgrGfx->findPhysicalDeviceMemoryType(
      outBuffer->memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(logicalDevice.device, &allocInfo, nullptr, &outMemory) !=
      VK_SUCCESS) {
    RE_LOG(Error, "Failed to allocate video memory for the buffer.");
    return RE_ERROR;
  }

  if (vkBindBufferMemory(logicalDevice.device, outBuffer->buffer, outMemory,
                         NULL) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to bind buffer memory.");
    return RE_ERROR;
  }*/

  return RE_OK;
}

TResult MGraphics::allocateBufferMemory(RBuffer* inBuffer,
                                               VkMemoryPropertyFlags properties,
                                               VkDeviceMemory& outMemory) {
  /*if (!inBuffer) {
    RE_LOG(Error, "No source buffer was provided.");
    return RE_ERROR;
  }

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = inBuffer->memRequirements.size;
  allocInfo.memoryTypeIndex = mgrGfx->findPhysicalDeviceMemoryType(
      inBuffer->memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(logicalDevice.device, &allocInfo, nullptr, &outMemory) !=
      VK_SUCCESS) {
    RE_LOG(Error, "Failed to allocate video memory.");
    return RE_ERROR;
  }

  if (vkBindBufferMemory(logicalDevice.device, inBuffer->buffer, outMemory,
                         NULL) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to bind buffer memory.");
    return RE_ERROR;
  }

  return RE_OK;
}

TResult MGraphics::copyToCPUAccessMemory(RBuffer* inBuffer,
                                         VkDeviceMemory& outMemory) {
  void* pMappedMemory;
  if (vkMapMemory(logicalDevice.device, outMemory, 0, inBuffer->size, NULL,
                  &pMappedMemory) != VK_SUCCESS) {
    return RE_ERROR;
  };
  memcpy(pMappedMemory, inBuffer->pData, (size_t)inBuffer->size);
  vkUnmapMemory(logicalDevice.device, outMemory);
  */
  return RE_OK;
}

TResult MGraphics::copyBuffer(RBuffer* srcBuffer, RBuffer* dstBuffer) {
  /*VkCommandBufferAllocateInfo cmdBufferInfo{};
  cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdBufferInfo.commandBufferCount = 1;
  cmdBufferInfo.commandPool = dataRender.commandPool;

  VkCommandBuffer cmdBuffer;
  vkAllocateCommandBuffers(logicalDevice.device, &cmdBufferInfo, &cmdBuffer);

  VkCommandBufferBeginInfo cmdBufferBeginInfo{};
  cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = srcBuffer->size;

  vkCmdCopyBuffer(cmdBuffer, srcBuffer->buffer, dstBuffer->buffer, 1,
                  &copyRegion);

  vkEndCommandBuffer(cmdBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;

  vkQueueSubmit(logicalDevice.queues.graphics, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(logicalDevice.queues.graphics);

  vkFreeCommandBuffers(logicalDevice.device, dataRender.commandPool, 1,
                       &cmdBuffer);
*/
  return RE_OK;
}