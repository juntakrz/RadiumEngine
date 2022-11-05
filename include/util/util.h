#pragma once

using json = nlohmann::json;

std::vector<char> readFile(const wchar_t* filename);
void processMessage(char level, const char* message, ...);
void validate(TResult result);
std::string wstrToStr(const wchar_t* string);

TResult jsonLoad(const wchar_t* path, json* out_j = nullptr) noexcept;