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

  ktxTexture* pKTXTexture = nullptr;
  KTX_error_code ktxResult;
  RVkLogicalDevice* logicalDevice = &core::renderer.logicalDevice;

  ktxVulkanDeviceInfo* deviceInfo = ktxVulkanDeviceInfo_Create(
      core::renderer.physicalDevice.device, logicalDevice->device,
      logicalDevice->queues.transfer, core::renderer.getCommandPool(1),
      nullptr);

  if (!deviceInfo) {
    RE_LOG(Error, "Failed to retrieve Vulkan device info.");
    return;
  }

  ktxResult = ktxTexture_CreateFromNamedFile(
      filePath.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &pKTXTexture);

  if (ktxResult != KTX_SUCCESS) {
    RE_LOG(Error, "Failed reading texture. KTX error %d.", ktxResult);
    return;
  }

  // create a texture record in the manager
  if (!m_textures.try_emplace(textureName).second) {
    // already loaded
#ifndef NDEBUG
    RE_LOG(Warning, "Texture \"%s\" already loaded.", textureName.c_str());
#endif
    return;
  }

  // prepare freshly created texture structure
  RTexture* newTexture = &m_textures.at(textureName);
  newTexture->name = textureName;
  newTexture->filePath = filePath;

  ktxResult = ktxTexture_VkUploadEx(
      pKTXTexture, deviceInfo, &newTexture->texture, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  if (ktxResult != KTX_SUCCESS) {
    RE_LOG(Error, "Failed uploading texture to GPU. KTX error %d.", ktxResult);
    revert(textureName.c_str());

    return;
  }

  ktxTexture_Destroy(pKTXTexture);
  ktxVulkanDeviceInfo_Destruct(deviceInfo);

  RE_LOG(Log, "Successfully loaded texture \"%s\".", textureName.c_str());
  /*
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

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

  VkMemoryRequirements memoryRequirements{};

  if (useStagingBuffer) {
    RBuffer stagingBuffer{};
    if (core::renderer.createBuffer(EBufferMode::STAGING,
                                    newTextureData.dataSize, stagingBuffer,
                                    nullptr) != RE_OK) {
      RE_LOG(Error, "Failed to create staging buffer for the new texture.");
      revert(textureName.c_str());
    }

    vkGetBufferMemoryRequirements(core::renderer.logicalDevice.device,
                                  stagingBuffer.buffer, &memoryRequirements);
  }*/
}

void core::MMaterials::destroyAllTextures() { m_textures.clear(); }

core::MMaterials::RTexture::~RTexture() {
  ktxVulkanTexture_Destruct(&texture, core::renderer.logicalDevice.device,
                            nullptr);
}