#include "pch.h"
#include "util/util.h"
#include "config.h"
#include "core/core.h"

namespace util {
std::vector<char> readFile(const wchar_t* filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    RE_LOG(Warning, "failed to open \"%s\".", filename);

    return std::vector<char>{};
  }

  size_t fileSize = file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

#ifndef NDEBUG
  char path[256];
  wcstombs(path, filename, 255);
  RE_LOG(Log, "Done reading file at \"%s\".", path);
#endif

  return buffer;
}

std::vector<char> readFile(const std::string filename) {
  std::ifstream file(filename.c_str(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    RE_LOG(Warning, "Failed to open \"%s\" for reading.", filename.c_str());

    return std::vector<char>{};
  }

  size_t fileSize = file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

#ifndef NDEBUG
  RE_LOG(Log, "Done reading file at \"%s\".", filename.c_str());
#endif

  return buffer;
}

TResult writeFile(const std::string& filename, const std::string& folder,
                  const char* pData, const int32_t dataSize) {
  if (filename.empty()) {
    RE_LOG(Error, "Failed to write file, the file name is empty.");
    return RE_ERROR;
  }

  if (!pData) {
    RE_LOG(Error,
           "Failed to write file '%s' to path '%s'. No data to write was "
           "provided.",
           filename.c_str(), folder.c_str());
    return RE_ERROR;
  }

  if (!folder.empty() && !std::filesystem::exists(folder)) {
    RE_LOG(Error,
           "Failed to write file '%s' to path '%s'. The target directory "
           "does not exist.",
           filename.c_str(), folder.c_str());
    return RE_ERROR;
  }

  const std::string& writePath = folder.empty() ? filename : folder + filename;

  std::ofstream file(writePath, std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    RE_LOG(Error, "Failed to open '%s' for writing.", writePath.c_str());
    return RE_ERROR;
  }

  file.write(pData, dataSize);

  RE_LOG(Log, "Successfully written file to '%s', size %d.", writePath.c_str(),
         dataSize);

  return RE_OK;
}

void processMessage(char level, const char* message, ...) {
  char buffer[256];
  va_list args;

  va_start(args, message);
  vsprintf(buffer, message, args);
  va_end(args);

  switch (level) {
    case RE_OK: {
      std::cout << "Log: " << buffer << "\n";
      break;
    }
    case RE_WARNING: {
      std::cout << "Warning: " << buffer << "\n";
      break;
    }
    case RE_ERROR: {
      std::cout << "ERROR: " << buffer << "\n";
      break;
    }
    case RE_CRITICAL: {
      std::cout << "CRITICAL ERROR!\n";
      throw std::runtime_error(buffer);
      break;
    }
  }
}

void validate(TResult result) {
  if (result > RE_ERRORLIMIT) {
    RE_LOG(Critical, "Terminating program");
    core::stop(result);
  }
}

std::string toString(const wchar_t* string) {
  char newString[_MAX_PATH];
  size_t newLength = wcslen(string) + 1;

  if (newLength > _MAX_PATH) newLength = _MAX_PATH;

  memset(newString, 0, newLength);
  wcstombs(newString, string, newLength - 1);

  return newString;
}

std::wstring toWString(const char* string) {
  wchar_t newWStr[_MAX_PATH];
  size_t newLength = strlen(string) + 1;

  if (newLength > _MAX_PATH) newLength = _MAX_PATH;

  memset(newWStr, 0, newLength);
  mbstowcs(newWStr, string, newLength);

  return newWStr;
}
VkDeviceSize getVulkanAlignedSize(VkDeviceSize originalSize,
                                  VkDeviceSize minAlignment) {
  return (originalSize + minAlignment - 1) & ~(minAlignment - 1);
}
}  // namespace util