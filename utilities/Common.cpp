#include "Common.hpp"

#include <fstream>
#include <sstream>

namespace Utilities {
std::string readFile(const std::filesystem::path& filePath)
{
    std::ifstream ifs(filePath.string(), std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        GENERATE_EXCEPTION("No such file or directory: {}", filePath.string());
    }
    std::ostringstream data;
    data << ifs.rdbuf();
    // NOTE: probably there is more efficient way to do this
    auto strData = data.str();
    if (strData.back() == '\n') {
        strData.pop_back();
    }
    return strData;
}

void writeToFile(const std::filesystem::path& filePath, const std::string& data,
                 bool newLine)
{
    std::ofstream ofs(filePath.string(), std::ios::out | std::ios::binary);
    ofs.write(data.c_str(), data.size());
    if (newLine) {
        ofs.put('\n');
    }
}

void writeToFile(const std::filesystem::path& filePath, const GitHash& hash)
{
    writeToFile(filePath, hash.data(), true);
}

std::string convertTimeSinceEpoch(const std::string& timeSinceEpoch)
{
    time_t datetime = std::stol(timeSinceEpoch);
    auto humanReadableTime = std::string(ctime(&datetime));
    // remove `\n` at the end of the string
    humanReadableTime.pop_back();
    return humanReadableTime;
}

// Replace time_since_epoch UTC_offset with human readable date
std::string decodeDateIn(const std::string& gitTime)
{
    auto timeSinceEpochEnds = gitTime.find(' ');
    auto timeSinceEpoch = gitTime.substr(0, timeSinceEpochEnds);
    auto offset = gitTime.substr(timeSinceEpochEnds + 1);
    return fmt::format("{} {}", convertTimeSinceEpoch(timeSinceEpoch), offset);
}
}; // namespace Utilities