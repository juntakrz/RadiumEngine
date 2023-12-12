#pragma once

namespace util {
std::vector<uint8_t> readFile(const wchar_t* filename);
std::vector<uint8_t> readFile(const char* filename);
TResult writeFile(const std::string& filename, const std::string& folder,
                  const char* pData, const int32_t dataSize);
void processMessage(char level, const char* message, ...);
void validate(TResult result);
std::string toString(const wchar_t* string);
std::wstring toWString(const char* string);

float random(float min, float max);
}  // namespace util