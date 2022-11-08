#include "pch.h"
#include "core/world/mesh/mesh.h"

void WMesh::setMemory() {
  vkGetBufferMemoryRequirements(mgrGfx->logicalDevice.device, vertexBuffer,
                                &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = mgrGfx->findPhysicalDeviceMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

}

void WMesh::destroy() {
  vkDestroyBuffer(mgrGfx->logicalDevice.device, vertexBuffer, nullptr);
}
