#pragma once

#include "vk_mem_alloc.h"

class WMesh;

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
  glm::vec3 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDesc();
  static std::vector<VkVertexInputAttributeDescription> getAttributeDescs();
};

struct NextVertex {
  glm::vec3 pos;        // POSITION
  glm::vec2 tex;        // TEXCOORD
  glm::vec3 normal;     // NORMAL
  glm::vec3 tangent;    // TANGENT
  glm::vec3 binormal;   // BINORMAL
};

// uniform buffer object for vertex shader (model * view * projection)
struct RMVPMatrices {
  glm::mat4 matModel      = glm::mat4(1.0f);
  glm::mat4 matView       = glm::mat4(1.0f);
  glm::mat4 matProjection = glm::mat4(1.0f);
};

struct RCameraSettings {
  float aspectRatio = 16.0f / 9.0f;
  float FOV = 90.0f;
  float nearZ = 0.1f;
  float farZ = 1000.0f;
};

struct WMeshData {
  uint32_t id;
  std::string name;
  std::string material;
  std::unique_ptr<WMesh> pMesh;       // visible main mesh
  std::unique_ptr<WMesh> pAuxMesh;    // simpler mesh used for occlusion testing/collision etc.
};

enum class EBCMode {    // VkBuffer creation mode
  CPU_UNIFORM = 0,      // create uniform buffer for GPU programs
  CPU_VERTEX,           // create vertex buffer for the iGPU (UNUSED)
  CPU_INDEX,            // create index buffer for the iGPU (UNUSED)
  DGPU_VERTEX,          // create dedicated GPU vertex buffer
  DGPU_INDEX            // create dedicated GPU index buffer
};

enum class EAType {     // actor type
  BASE = 0,
  CAMERA,
  MODEL
};