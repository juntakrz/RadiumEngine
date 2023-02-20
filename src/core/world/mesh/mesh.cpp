#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/mrenderer.h"
#include "core/world/mesh/mesh.h"

WMesh::WMesh() {
  vertexBuffer.buffer = VK_NULL_HANDLE;
  vertexBuffer.allocation = VK_NULL_HANDLE;

  indexBuffer.buffer = VK_NULL_HANDLE;
  indexBuffer.allocation = VK_NULL_HANDLE;
};

void WMesh::createVertexBuffer() {
  VkDeviceSize size = vertices.size() * sizeof(RVertex);
  core::graphics.createBuffer(EBCMode::DGPU_VERTEX, size, vertexBuffer.buffer,
                       vertexBuffer.allocation, vertices.data(),
                       &vertexBuffer.allocInfo);
}

void WMesh::createIndexBuffer() {
  VkDeviceSize size = indices.size() * sizeof(indices[0]);
  core::graphics.createBuffer(EBCMode::DGPU_INDEX, size, indexBuffer.buffer,
                       indexBuffer.allocation, indices.data(),
                       &indexBuffer.allocInfo);
}

void WMesh::allocateMemory() {
  createVertexBuffer();
  createIndexBuffer();
}

void WMesh::destroy() {
  vmaDestroyBuffer(core::graphics.memAlloc, vertexBuffer.buffer,
                   vertexBuffer.allocation);
  vmaDestroyBuffer(core::graphics.memAlloc, indexBuffer.buffer,
                   indexBuffer.allocation);
}