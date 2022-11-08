#include "pch.h"
#include "core/world/mesh/mesh_plane.h"
#include "core/world/primitives/plane.h"

void WMesh_Plane::create(int arg1, int arg2) {
  auto model = WPrimitive_Plane::create<RVertex>(arg1, arg2);
  vertices = model.vertices;
  indices = model.indices;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.size = sizeof(RVertex) * vertices.size();
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferInfo.flags = NULL;

  if(vkCreateBuffer(mgrGfx->logicalDevice.device, &bufferInfo, nullptr,
                     &vertexBuffer) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create vertex buffer for plane mesh.");
  };

  setMemory();
}
