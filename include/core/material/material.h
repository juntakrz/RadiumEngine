#pragma once

#include "core/objects.h"

struct RTexture;

struct RMaterial {
  std::string name;
  bool doubleSided = false;
  EAlphaMode alphaMode;

  std::string shaderVertex, shaderPixel, shaderGeometry;
  RTexture* pBaseColor = nullptr;
  RTexture* pNormal = nullptr;
  RTexture* pMetalRoughness = nullptr;
  RTexture* pOcclusion = nullptr;
  RTexture* pEmissive = nullptr;
  RTexture* pExtra = nullptr;

  RMaterialPCB pushConstantBlock;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  // add effects using bitwise ops
  uint32_t effectFlags;

  // will materials manager automatically try to delete textures
  // from memory if unused by any other material
  // !requires sharedPtr code, currently unused!
  bool manageTextures = false;

  void createDescriptorSet();
};