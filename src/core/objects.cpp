#include "pch.h"
#include "core/objects.h"

std::set<int32_t> RVkQueueFamilyIndices::getAsSet() const {
  if (graphics.empty() || compute.empty() || present.empty() ||
      transfer.empty()) {
    return {-1};
  }

  return {graphics.at(0), compute.at(0), present.at(0), transfer.at(0)};
}

VkVertexInputBindingDescription RVertex::getBindingDesc() {
    VkVertexInputBindingDescription bindDesc{};
    bindDesc.binding = 0;
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindDesc.stride = sizeof(RVertex);

    return bindDesc;
}

std::vector<VkVertexInputAttributeDescription> RVertex::getAttributeDescs() {
  std::vector<VkVertexInputAttributeDescription> attrDescs(5);
  
  // describes 'pos'
  attrDescs[0].binding = 0;                         // binding defined by binding description of RVertex
  attrDescs[0].location = 0;                        // input location in vertex shader
  attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT; // 'pos' consists of 3x 32 bit floats
  attrDescs[0].offset = offsetof(RVertex, pos);     // offset of 'pos' in memory, in bytes

  // describes 'tex'
  attrDescs[1].binding = 0;
  attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT;    // UV coordinates
  attrDescs[1].location = 1;
  attrDescs[1].offset = offsetof(RVertex, tex);

  // describes 'normal'
  attrDescs[2].binding = 0;
  attrDescs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrDescs[2].location = 2;
  attrDescs[2].offset = offsetof(RVertex, normal);

  // describes 'tangent'
  attrDescs[3].binding = 0;
  attrDescs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrDescs[3].location = 3;
  attrDescs[3].offset = offsetof(RVertex, tangent);

  // describes 'binormal'
  attrDescs[4].binding = 0;
  attrDescs[4].format = VK_FORMAT_R32G32B32_SFLOAT;  // UV coordinates
  attrDescs[4].location = 4;
  attrDescs[4].offset = offsetof(RVertex, binormal);

  return attrDescs;
}
