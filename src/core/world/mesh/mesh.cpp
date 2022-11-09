#include "pch.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"
#include "core/world/mesh/mesh.h"

void WMesh::setMemory() {

  if (mgrGfx->createLogicalDeviceBuffer(vertices.size() * sizeof(RVertex),
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    &vertexBuffer) != RE_OK){
    RE_LOG(Critical, "Failed to create vertex buffer for mesh.");
    return;
  };

  if (mgrGfx->allocateLogicalDeviceMemory(
          &vertexBuffer,
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
          mgrGfx->logicalDevice.vertexBufferMemory) != RE_OK) {
    RE_LOG(Critical, "Failed to allocate device memory for the vertex buffer.");
    return;
  };
}

void WMesh::destroy() {
  vkDestroyBuffer(mgrGfx->logicalDevice.device, vertexBuffer.buffer, nullptr);
}
