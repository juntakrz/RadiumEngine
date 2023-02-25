#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/world/model/mesh.h"

WMesh::WMesh() {
  vertexBuffer.buffer = VK_NULL_HANDLE;
  vertexBuffer.allocation = VK_NULL_HANDLE;

  indexBuffer.buffer = VK_NULL_HANDLE;
  indexBuffer.allocation = VK_NULL_HANDLE;
};

void WMesh::createVertexBuffer(const std::vector<RVertex>& vertexData) {
  VkDeviceSize size = vertexData.size() * sizeof(RVertex);
  core::renderer.createBuffer(EBCMode::DGPU_VERTEX, size, vertexBuffer,
                              (void*)vertexData.data());
  vertexCount = static_cast<uint32_t>(vertexData.size());
}

void WMesh::createIndexBuffer(const std::vector<uint32_t>& indexData) {
  VkDeviceSize size = indexData.size() * sizeof(indexData[0]);
  core::renderer.createBuffer(EBCMode::DGPU_INDEX, size, indexBuffer,
                              (void*)indexData.data());
  indexCount = static_cast<uint32_t>(indexData.size());
}

void WMesh::createBuffers(const std::vector<RVertex>& vertexData,
                          const std::vector<uint32_t>& indexData) {
  createVertexBuffer(vertexData);
  createIndexBuffer(indexData);
}

void WMesh::destroy() {
  vmaDestroyBuffer(core::renderer.memAlloc, vertexBuffer.buffer,
                   vertexBuffer.allocation);
  vmaDestroyBuffer(core::renderer.memAlloc, indexBuffer.buffer,
                   indexBuffer.allocation);
}