#pragma once

#include <filesystem>
#include <string>

namespace Zlib {

std::string compress(const std::string& data);
std::string decompress(const std::string& data);

std::string compressFile(const std::filesystem::path& filePath);
std::string decompressFile(const std::filesystem::path& filePath);

std::string readFile(const std::filesystem::path& filePath);
void writeToFile(const std::filesystem::path& filePath, const std::string& data);

}; // namespace Zlib