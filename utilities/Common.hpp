#pragma once

#include <filesystem>
#include <fmt/core.h>
#include <string>

#include "../git_objects/GitHash.hpp"

namespace Utilities {
#define GENERATE_EXCEPTION(MSG, ...)                                           \
    throw std::runtime_error(fmt::format(MSG, __VA_ARGS__));

#define DEBUG(value) std::cout << #value << ": " << value << std::endl;

std::string readFile(const std::filesystem::path& filePath);
void writeToFile(const std::filesystem::path& filePath,
                 const std::string& data, bool newLine = false);
void writeToFile(const std::filesystem::path& filePath,
                 const GitHash& hash);

std::string decodeDateIn(const std::string& gitTime);
}; // namespace Utilities