#include "pch.h"
#include "core/objects.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/materials.h"

void core::MMaterials::loadTexture(const std::string& filePath) {
  auto revert = [&](const char* name) { m_textures.erase(name); };

  if (filePath == "") {
    // nothing to load
    return;
  }

  // generate texture name from path - no texture file can be named the same
  // size_t with npos will overflow to 0 when 1 is added, which is good
  size_t folderPosition = filePath.find_last_of("/\\");
  size_t delimiterPosition = filePath.rfind('.');

  if (delimiterPosition == std::string::npos) {
    RE_LOG(Warning, "Delimiter is missing in file path \"%s\".",
           filePath.c_str());
    delimiterPosition = filePath.size() - 1;
  }

  std::string textureName(filePath, folderPosition + 1,
                          delimiterPosition - (folderPosition + 1));

  if (!m_textures.try_emplace(textureName).second) {
    // already loaded
#ifndef NDEBUG
    RE_LOG(Warning, "Texture \"%s\" already loaded.", textureName.c_str());
#endif
    return;
  }

  std::vector<uint8_t> rawData = util::readFile(filePath.c_str());
  size_t rawDataSize = rawData.size();

  if (rawDataSize < 1) {
    RE_LOG(Error, "Reading texture resulted in no data.", filePath.c_str());
    revert(textureName.c_str());
    return;
  }


  // create texture (must be of either KTX1 or KTX2 format)
  ktxTexture* pKTXTexture = nullptr;
  KTX_error_code ktxResult;

  ktxResult = ktxTexture_CreateFromNamedFile(
      filePath.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &pKTXTexture);

  /*ktxResult = ktxTexture_CreateFromMemory(
      rawData.data(), rawDataSize, KTX_TEXTURE_CREATE_NO_FLAGS,
      &newTexture);

  if (ktxResult != KTX_SUCCESS || !newTexture->pData) {
    RE_LOG(Error, "Failed reading texture. Possibly unknown format.");
    return;
  }

  // prepare freshly created texture structure
  RTexture& newTextureData = m_textures.at(textureName);
  newTextureData.name = textureName;
  newTextureData.filePath = filePath;
  newTextureData.width = newTexture->baseWidth;
  newTextureData.height = newTexture->baseHeight;
  newTextureData.dataSize = newTexture->dataSize;
  newTextureData.mipLevels = newTexture->numLevels;

  // prepare data for loading texture into the graphics card memory
  // assume KTX1 uses sRGB format instead of linear (most tools use this)
  VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

  // using staging buffer (possibly test if it's not needed after all)
  bool useStagingBuffer = true;
  bool forceLinearTiling = false;

  // don't use format if linear shader sampling is not supported
  if (forceLinearTiling) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(core::renderer.physicalDevice.device,
                                        format, &formatProperties);
    useStagingBuffer = !(formatProperties.linearTilingFeatures &
                    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
  }

  uint8_t* pTextureData = newTexture->pData;

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

  VkMemoryRequirements memoryRequirements{};
  
  if (useStagingBuffer) {
    RBuffer stagingBuffer{};
    if (core::renderer.createBuffer(EBufferMode::STAGING,
                                    newTextureData.dataSize, stagingBuffer,
                                    pTextureData) != RE_OK) {
      RE_LOG(Error, "Failed to create staging buffer for the new texture.");
      revert(textureName.c_str());
    }

    vkGetBufferMemoryRequirements(core::renderer.logicalDevice.device,
                                  stagingBuffer.buffer, &memoryRequirements);
  }*/
}