#pragma once

namespace util {
std::vector<char> readFile(const wchar_t* filename);
std::vector<char> readFile(const std::string filename);
TResult writeFile(const std::string& filename, const std::string& folder,
                  const char* pData, const int32_t dataSize);
void processMessage(char level, const char* message, ...);
void validate(TResult result);
std::string toString(const wchar_t* string);
std::wstring toWString(const char* string);
const VkDeviceSize getVulkanAlignedSize(VkDeviceSize originalSize,
                                        VkDeviceSize minAlignmanet);

void copyVec3ToMatrix(const float* vec3, glm::mat4& matrix, const uint8_t column) noexcept;

template<typename T>
size_t hash(T input) {
  std::hash<T> hasher;
  return hasher(input);
}
}  // namespace util