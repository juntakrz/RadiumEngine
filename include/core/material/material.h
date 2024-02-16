#pragma once

#include "core/objects.h"

struct RTexture;

struct RMaterial {
  std::string name;
  uint32_t passFlags = EDynamicRenderingPass::Null;
  std::vector<RTexture*> pLinearTextures;

  RTexture* pBaseColor = nullptr;
  RTexture* pNormal = nullptr;
  RTexture* pMetalRoughness = nullptr;
  RTexture* pOcclusion = nullptr;
  RTexture* pEmissive = nullptr;
  RTexture* pExtra0 = nullptr;
  RTexture* pExtra1 = nullptr;
  RTexture* pExtra2 = nullptr;

  RSceneFragmentPCB pushConstantBlock;

  // will materials manager automatically try to delete textures
  // from memory if unused by any other material
  // !requires sharedPtr code, currently unused!
  bool manageTextures = false;
};