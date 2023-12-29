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
uint32_t config::shadowResolution = 2048u;
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
  return RE_NODEDATASIZE * nodeBudget;
}

size_t config::scene::getSkinTransformBufferSize() {
  return RE_SKINDATASIZE * entityBudget;
}

VkFormat core::vulkan::formatDepth;
VkFormat core::vulkan::formatShadow;
VkDeviceSize core::vulkan::minBufferAlignment = 64u;