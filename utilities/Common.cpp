#include "Common.hpp"

#include <fstream>
#include <sstream>

namespace Utilities {
std::string readFile(const std::filesystem::path& filePath)
{
    std::ifstream ifs(filePath.string(), std::ios::in | std::ios::binary);
    std::ostringstream data;
    data << ifs.rdbuf();
    return data.str();
}

void writeToFile(const std::filesystem::path& filePath, const std::string& data)
{
    std::ofstream ofs(filePath.string(), std::ios::out | std::ios::binary);
    ofs.write(data.c_str(), data.size());
}
};