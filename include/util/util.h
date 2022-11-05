#pragma once

std::vector<char> readFile(const std::wstring& filename);
void processMessage(char level, const char* message, ...);
void validate(TResult result);