#pragma once

std::vector<char> readFile(const wchar_t* filename);
void processMessage(char level, const char* message, ...);
void validate(TResult result);
std::string wstrToStr(const wchar_t* string);

float random(float min, float max);