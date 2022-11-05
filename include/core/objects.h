#pragma once

struct CVkQueueFamilyIndices {
  std::vector<int32_t> graphics;
  std::vector<int32_t> compute;
  std::vector<int32_t> present;

  // retrieve first entries for queue family indices
  std::set<int32_t> getAsSet() const;
};

struct CVkSwapChainInfo {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> modes;
};

struct CVkPhysicalDevice {
  VkPhysicalDevice device = VK_NULL_HANDLE;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceProperties properties;
  CVkQueueFamilyIndices queueFamilyIndices;
  CVkSwapChainInfo swapChainInfo;
  bool bIsValid = false;
};

struct CVkLogicalDevice {
  VkDevice device;
  struct {
    VkQueue graphics = VK_NULL_HANDLE;
    VkQueue compute = VK_NULL_HANDLE;
    VkQueue present = VK_NULL_HANDLE;
  } queues;
};