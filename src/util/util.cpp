#include "pch.h"
#include "util/util.h"
#include "settings.h"
#include "core/core.h"

std::vector<char> readFile(const std::wstring& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    RE_LOG(RE_WARNING, "failed to open '%s'.", filename.c_str());

    return std::vector<char>{};
  }

  size_t fileSize = file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  char path[256];
  wcstombs(path, filename.c_str(), 255);
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
