#include "pch.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"
#include "core/world/mesh/mesh.h"

void WMesh::setMemory () {
  vkGetBufferMemoryRequirements(mgrGfx->logicalDevice.device,
                                vertexBuffer.buffer,
                                &vertexBuffer.memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = vertexBuffer.memRequirements.size;
  allocInfo.memoryTypeIndex = mgrGfx->findPhysicalDeviceMemoryType (
    vertexBuffer.memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (mgrGfx->allocateLogicalDeviceMemory(&allocInfo, &vertexBuffer) != RE_OK) {
    RE_LOG(Critical, "Failed to allocate device memory for the vertex buffer.");
    return;
  };
}

void WMesh::destroy() {
  vkDestroyBuffer(mgrGfx->logicalDevice.device, vertexBuffer.buffer, nullptr);
}
