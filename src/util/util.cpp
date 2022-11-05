#include "pch.h"
#include "util/util.h"
#include "config.h"
#include "core/core.h"

std::vector<char> readFile(const wchar_t* filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    RE_LOG(RE_WARNING, "failed to open '%s'.", filename);

    return std::vector<char>{};
  }

  size_t fileSize = file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  char path[256];
  wcstombs(path, filename, 255);
  RE_LOG(Log, "Successfully loaded '%s'.", path);

  return buffer;
}

void processMessage(char level, const char* message, ...) {
  char buffer[256];
  va_list args;

  va_start(args, message);
  vsprintf(buffer, message, args);
  va_end(args);

  switch (level) {
    case RE_OK:
    {
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
};

void validate(TResult result) {
  if (result > RE_ERRORLIMIT) {
    RE_LOG(Critical, "Terminating program");
    core::stop(result);
  }
}

std::string wstrToStr(const wchar_t* string) {
  char newString[255];
  size_t newLength = wcslen(string) + 1;

  if (newLength > 255) newLength = 255;

  memset(newString, 0, newLength);
  wcstombs(newString, string, newLength - 1);
  
  return newString;
}

TResult jsonLoad(const wchar_t* path, json* out_j) noexcept {
  if (!path) {
    RE_LOG(Error, "jsonLoad received empty path.");
    return RE_ERROR;
  }

  std::ifstream fStream(path);
  out_j->clear();

  if (!fStream.good()) {
    RE_LOG(Error, "Failed to read JSON file at '%s'.", path);
    return RE_ERROR;
  }

  fStream >> *out_j;
  return RE_OK;
}