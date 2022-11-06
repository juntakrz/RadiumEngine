#pragma once

class WMesh;

struct RVkQueueFamilyIndices {
  std::vector<int32_t> graphics;
  std::vector<int32_t> compute;
  std::vector<int32_t> present;

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
  RVkQueueFamilyIndices queueFamilyIndices;
  RVkSwapChainInfo swapChainInfo;
  bool bIsValid = false;
};

struct RVkLogicalDevice {
  VkDevice device;
  struct {
    VkQueue graphics = VK_NULL_HANDLE;
    VkQueue compute = VK_NULL_HANDLE;
    VkQueue present = VK_NULL_HANDLE;
  } queues;
};

struct RVertex {
  glm::vec3 pos;
  glm::vec3 color;
};

struct NextVertex {
  glm::vec3 pos;        // POSITION
  glm::vec2 tex;        // TEXCOORD
  glm::vec3 normal;     // NORMAL
  glm::vec3 tangent;    // TANGENT
  glm::vec3 binormal;   // BINORMAL
};

struct WMeshData {
  uint32_t id;
  std::string name;
  std::string material;
  std::unique_ptr<WMesh> pMesh;       // visible main mesh
  std::unique_ptr<WMesh> pAuxMesh;    // simpler mesh used for occlusion testing/collision etc.
};