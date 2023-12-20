#include "pch.h"
#include "core/objects.h"
#include "config.h"

const char* config::appTitle = "Radium Engine";
const char* config::engineTitle = "Radium Engine";
uint32_t config::renderWidth = 1280;
uint32_t config::renderHeight = 720;
float config::viewDistance = 1000.0f;
float config::FOV = 90.0f;
bool config::bDevMode = false;
float config::pitchLimit = glm::radians(88.0f);

float config::getAspectRatio() { return renderWidth / (float)renderHeight; }

size_t config::scene::getVertexBufferSize() {
  return sizeof(RVertex) * vertexBudget;
}

size_t config::scene::getIndexBufferSize() {
  return sizeof(uint32_t) * indexBudget;
}

size_t config::scene::getRootTransformBufferSize() {
  return sizeof(glm::mat4) * 2 * entityBudget;
}

size_t config::scene::getNodeTransformBufferSize() {
  return sizeof(glm::mat4) * 4 * entityBudget;
}

VkFormat core::vulkan::formatDepth;