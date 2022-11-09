#include "pch.h"
#include "core/world/mesh/mesh.h"
#include "core/world/mesh/mesh_plane.h"
#include "core/world/primitives/plane.h"
#include "core/core.h"
#include "core/managers/mgraphics.h"

void WMesh_Plane::create(int arg1, int arg2) {
  auto model = WPrimitive_Plane::create<RVertex>(arg1, arg2);
  vertices = model.vertices;
  indices = model.indices;

  vertexBuffer.pData = vertices.data();

  vertexBuffer.bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBuffer.bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vertexBuffer.bufferInfo.size = sizeof(RVertex) * vertices.size();
  vertexBuffer.bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vertexBuffer.bufferInfo.flags = NULL;

  if (vkCreateBuffer(mgrGfx->logicalDevice.device, &vertexBuffer.bufferInfo,
                     nullptr, &vertexBuffer.buffer) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to create vertex buffer for plane mesh.");
    return;
  };

  setMemory();
}
