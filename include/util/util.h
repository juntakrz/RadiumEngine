#pragma once

std::vector<char> readFile(const wchar_t* filename);
void processMessage(char level, const char* message, ...);
void validate(TResult result);
std::string toString(const wchar_t* string);
std::wstring toWString(const char* string);

float random(float min, float max);