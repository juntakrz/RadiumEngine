#pragma once

#include "vk_mem_alloc.h"
#include "config.h"

enum class EBCMode {  // VkBuffer creation mode
  CPU_UNIFORM,        // create uniform buffer for GPU programs
  CPU_VERTEX,         // create vertex buffer for the iGPU (UNUSED)
  CPU_INDEX,          // create index buffer for the iGPU (UNUSED)
  DGPU_VERTEX,        // create dedicated GPU vertex buffer
  DGPU_INDEX          // create dedicated GPU index buffer
};

enum class EAType {  // actor type
  Base,
  Camera,
  Model
};

struct RVkQueueFamilyIndices {
  std::vector<int32_t> graphics;
  std::vector<int32_t> compute;
  std::vector<int32_t> present;
  std::vector<int32_t> transfer;

  // retrieve first entries for queue family indices
  std::set<int32_t> getAsSet() const;
};

struct RVkSwapChainInfo {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> modes;
};

struct RVkPhysicalDevice {
  VkPhysicalDevice device = VK_NULL_HANDLE;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceMemoryProperties memProperties;
  RVkQueueFamilyIndices queueFamilyIndices;
  RVkSwapChainInfo swapChainInfo;
  bool bIsValid = false;
};

struct RVkLogicalDevice {
  VkDevice device;

  struct {
    VkQueue graphics  = VK_NULL_HANDLE;
    VkQueue compute   = VK_NULL_HANDLE;
    VkQueue present   = VK_NULL_HANDLE;
    VkQueue transfer  = VK_NULL_HANDLE;
  } queues;
};

struct RBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
};

struct RVertex {
  glm::vec3 pos;        // POSITION
  glm::vec2 tex0;       // TEXCOORD0
  glm::vec2 tex1;       // TEXCOORD1
  glm::vec3 normal;     // NORMAL
  glm::vec3 tangent;    // TANGENT
  glm::vec3 binormal;   // BINORMAL
  glm::vec4 color;      // COLOR
  glm::vec4 joint;      // JOINT
  glm::vec4 weight;     // WEIGHT

  static VkVertexInputBindingDescription getBindingDesc();
  static std::vector<VkVertexInputAttributeDescription> getAttributeDescs();
};

struct RCameraSettings {
  float aspectRatio = config::getAspectRatio();
  float FOV = config::FOV;
  float nearZ = RE_NEARZ;
  float farZ = config::viewDistance;
};

// uniform buffer objects
// 
// view matrix UBO for vertex shader (model * view * projection)
struct RSModelViewProjection {
  alignas(16) glm::mat4 model = glm::mat4(1.0f);
  alignas(16) glm::mat4 view = glm::mat4(1.0f);
  alignas(16) glm::mat4 projection = glm::mat4(1.0f);
};