#include "pch.h"
#include "core/world/mesh/mesh_plane.h"
#include "core/world/primitives/plane.h"

void WMesh_Plane::create(int arg1, int arg2) {
  auto model = WPrimitive_Plane::create<RVertex>(arg1, arg2);
  vertices = model.vertices;
  indices = model.indices;

  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vertexBufferInfo.size = sizeof(RVertex) * vertices.size();
  vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vertexBufferInfo.flags = NULL;

  if(vkCreateBuffer(mgrGfx->logicalDevice.device, &vertexBufferInfo, nullptr,
                     &vertexBuffer) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create vertex buffer for plane mesh.");
    return;
  };

  setMemory();
}
