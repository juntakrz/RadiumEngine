#include "pch.h"
#include "util/util.h"
#include "core/objects.h"

std::set<int32_t> RVkQueueFamilyIndices::getAsSet() const {
  if (graphics.empty() || compute.empty() || transfer.empty() ||
      present.empty()) {
    return {-1};
  }

  return {
      graphics.at(0),
      compute.at(0),
      transfer.at(0),
      present.at(0),
  };
}

VkVertexInputBindingDescription RVertex::getBindingDesc() {
    VkVertexInputBindingDescription bindDesc{};
    bindDesc.binding = 0;
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindDesc.stride = sizeof(RVertex);

    return bindDesc;
}

std::vector<VkVertexInputAttributeDescription> RVertex::getAttributeDescs() {
  std::vector<VkVertexInputAttributeDescription> attrDescs(8);
  
  // describes 'pos'
  attrDescs[0].binding = 0;                         // binding defined by binding description of RVertex
  attrDescs[0].location = 0;                        // input location in vertex shader
  attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT; // 'pos' consists of 3x 32 bit floats
  attrDescs[0].offset = offsetof(RVertex, pos);     // offset of 'pos' in memory, in bytes

  // describes 'tex0'
  attrDescs[1].binding = 0;
  attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT;    // UV coordinates
  attrDescs[1].location = 1;
  attrDescs[1].offset = offsetof(RVertex, tex0);

  // describes 'tex1'
  attrDescs[2].binding = 0;
  attrDescs[2].format = VK_FORMAT_R32G32_SFLOAT;    // UV coordinates
  attrDescs[2].location = 2;
  attrDescs[2].offset = offsetof(RVertex, tex1);

  // describes 'normal'
  attrDescs[3].binding = 0;
  attrDescs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrDescs[3].location = 3;
  attrDescs[3].offset = offsetof(RVertex, normal);

  // describes 'color'
  attrDescs[4].binding = 0;
  attrDescs[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attrDescs[4].location = 4;
  attrDescs[4].offset = offsetof(RVertex, color);

  // describes 'joint'
  attrDescs[5].binding = 0;
  attrDescs[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attrDescs[5].location = 5;
  attrDescs[5].offset = offsetof(RVertex, joint);

  // describes 'weight'
  attrDescs[6].binding = 0;
  attrDescs[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attrDescs[6].location = 6;
  attrDescs[6].offset = offsetof(RVertex, weight);

  // describes 'tangent'
  attrDescs[7].binding = 0;
  attrDescs[7].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrDescs[7].location = 7;
  attrDescs[7].offset = offsetof(RVertex, tangent);

  return attrDescs;
}

void RAsync::loop() {
  while (execute) {
    std::unique_lock<std::mutex> lock(mutex);
    conditional.wait(lock, [this]() { return cue || !execute; });

    if (!execute) {
      break;
    }

    cue = false;
    func->exec();
  }
}

void RAsync::start() {
  if (!func) {
    RE_LOG(Error, "Can't start async object, no function is bound.");
    return;
  }

  thread = std::thread(&RAsync::loop, this);
}

void RAsync::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex);
    execute = false;
  }
  conditional.notify_all();

  if (thread.joinable()) {
    thread.join();
  }
}

void RAsync::update() {
  std::unique_lock<std::mutex> lock(mutex);
  cue = true;
  conditional.notify_all();
}
