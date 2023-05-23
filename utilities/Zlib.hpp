#pragma once

#include <filesystem>
#include <string>

namespace Zlib {

std::string compress(const std::string& data);
void compress(const std::filesystem::path& filePath, const std::string& data);

std::string decompress(const std::string& data);
std::string decompressFile(const std::filesystem::path& filePath);
}; // namespace Zlib