#pragma once

#include "core/objects.h"

// NOTE: if texture is loaded using KTX library - VMA allocations are not used
struct RTexture {
  std::string name = "";
  RVulkanTexture texture;
  VmaAllocation allocation;
  VmaAllocationInfo allocationInfo;
  bool isKTX = false;
  bool isCubemap = false;

  // trying to track how many times texture is assigned to material
  // to see if it should be deleted if owning material is deleted
  uint32_t references = 0;

  TResult createImageView();
  TResult createSampler(RSamplerInfo *pSamplerInfo);
  TResult createDescriptor();
  ~RTexture();
};

struct RTextureCube : public RTexture {

};