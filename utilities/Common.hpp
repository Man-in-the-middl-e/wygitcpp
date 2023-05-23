#pragma once

#include <filesystem>
#include <fmt/core.h>
#include <string>

namespace Utilities {
#define GENERATE_EXCEPTION(MSG, ...)                                           \
    throw std::runtime_error(fmt::format(MSG, __VA_ARGS__));

std::string readFile(const std::filesystem::path& filePath);
void writeToFile(const std::filesystem::path& filePath,
                 const std::string& data);
}; // namespace Utilities