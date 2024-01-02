#include "pch.h"
#include "core/objects.h"
#include "util/util.h"
#include "config.h"

const char* config::appTitle = "Radium Engine";
const char* config::engineTitle = "Radium Engine";
uint32_t config::renderWidth = 1280u;
uint32_t config::renderHeight = 720u;
float config::viewDistance = 1000.0f;
float config::FOV = 90.0f;
bool config::bDevMode = false;
float config::pitchLimit = glm::radians(88.0f);
uint32_t config::shadowResolution = 4096u;
uint32_t config::shadowCascades = 3u;

float config::getAspectRatio() { return renderWidth / (float)renderHeight; }

size_t config::scene::getVertexBufferSize() {
  return sizeof(RVertex) * vertexBudget;
}

size_t config::scene::getIndexBufferSize() {
  return sizeof(uint32_t) * indexBudget;
}

size_t config::scene::getRootTransformBufferSize() {
  return sizeof(glm::mat4) * entityBudget;
}

size_t config::scene::getNodeTransformBufferSize() {
  return config::scene::nodeBlockSize * nodeBudget;
}

size_t config::scene::getSkinTransformBufferSize() {
  return config::scene::skinBlockSize * entityBudget;
}

size_t config::scene::getMaxCameraCount() { return cameraBudget; }

VkFormat core::vulkan::formatDepth;
VkFormat core::vulkan::formatShadow;
VkDeviceSize core::vulkan::minUniformBufferAlignment = 64u;
VkDeviceSize core::vulkan::descriptorBufferOffsetAlignment = 64u;

uint32_t config::scene::sampledImageBudget = 64u;
uint32_t config::scene::storageImageBudget = 64u;

uint32_t config::scene::cameraBlockSize = 0u;
uint32_t config::scene::nodeBlockSize = 0u;
uint32_t config::scene::skinBlockSize = 0u;