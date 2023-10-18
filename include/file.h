#ifndef FILE_H
#define FILE_H

#include "header.h"

extern std::string workdir;
extern std::string logPath;

bool IsFile(const std::string& filePath);

bool ReadFile(const std::string &filePath, std::string &fileData, bool lock);

bool WriteFile(const std::string &filePath, const std::string &fileData, bool lock);

void splitString(std::vector<std::string> &words, const std::string& str, char delim);

#endif