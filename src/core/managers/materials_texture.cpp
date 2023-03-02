#include "pch.h"
#include "core/objects.h"
#include "core/managers/materials.h"

#define DDSKTX_IMPLEMENT
#include <dds-ktx.h>

void core::MMaterials::loadTexture(const std::string& filePath) {
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

  std::vector<char> textureData = util::readFile(filePath.c_str());
  int32_t textureDataSize = static_cast<int32_t>(textureData.size());

  if (textureDataSize < 1) {
    RE_LOG(Error, "Reading texture resulted in no data.", filePath.c_str());
    return;
  }

  ddsktx_texture_info textureInfo{};
  ddsktx_error textureError{};
  ddsktx_parse(&textureInfo, textureData.data(), textureDataSize,
               &textureError);

  if (textureError.msg != "") {
    RE_LOG(Error, "Failed loading the texture. Error message: \"%s\"", textureError.msg);
    return;
  }
}