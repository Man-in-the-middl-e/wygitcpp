#include "GitIndex.hpp"
#include "../utilities/Common.hpp"

#include <cmath>
#include <fstream>
#include <iostream>

namespace {
using OneByte = uint8_t;
using TwoBytes = uint16_t;
using FourBytes = uint32_t;
using EightBytes = uint64_t;

template <class T> T convert(T bigEndian)
{
    T numberOfBytes = sizeof(T);
    T res = 0;
    char* data = reinterpret_cast<char*>(&bigEndian);
    for (int i = numberOfBytes - 1, byte = 0; i > 0; --i, ++byte) {
        res |= (data[i] << i * byte);
    }
    res |= (unsigned)(data[0] << (numberOfBytes - 1) * 8);
    return res;
}

template <class T> T read(std::fstream& ifs)
{
    T res;
    ifs.read(reinterpret_cast<char*>(&res), sizeof(T));
    return res;
}

uint16_t read2(std::fstream& ifs) { return read<uint16_t>(ifs); }

uint32_t read4(std::fstream& ifs) { return read<uint32_t>(ifs); }

uint64_t read8(std::fstream& ifs) { return read<uint64_t>(ifs); }

std::string readString(std::fstream& ifs, size_t stringSize)
{
    std::string res(stringSize, '\0');
    ifs.read(&res[0], stringSize);
    return res;
}

std::string readStringUntilZero(std::fstream& ifs)
{
    std::string res;
    std::getline(ifs, res, '\0');
    return res;
}

void padByByteBoundary(std::fstream& ifs, int numberOfZeros)
{
    std::string padding(numberOfZeros, '\0');
    ifs.read(&padding[0], numberOfZeros);
}

} // namespace
namespace Git {

std::vector<IndexEntry> GitIndex::parse(const std::string& indexFilePath)
{
    std::fstream ifs(indexFilePath, std::ios::binary | std::ios::in);
    if (!ifs.is_open()) {
        GENERATE_EXCEPTION("Coulnd't find index file: {}", indexFilePath);
    }
    if (auto signature = readString(ifs, 4); signature != "DIRC") {
        GENERATE_EXCEPTION("Wrong file signature expected 'DIRC', got {}",
                           signature);
    }

    if (auto version = convert<FourBytes>(read4(ifs)); version >= 0) {
        std::cout << "Current Git Index Version: " << version << std::endl;
    }
    else {
        GENERATE_EXCEPTION("Wrong version number {}", version);
    }

    if (auto numberOfIndices = convert<FourBytes>(read4(ifs));
        numberOfIndices >= 0) {
        std::cout << "Number of Git Indices: " << numberOfIndices << std::endl;
        std::vector<GitIndex> indices;
        indices.reserve(numberOfIndices);

        std::vector<IndexEntry> res;
        for (int i = 0; i < numberOfIndices; ++i) {
            int beginningOfTheEntry = ifs.tellp();
            res.push_back({.ctime = read8(ifs),
                           .mtime = read8(ifs),
                           .dev = read4(ifs),
                           .ino = read4(ifs),
                           .mode = read4(ifs),
                           .uid = read4(ifs),
                           .gid = read4(ifs),
                           .fsize = convert<FourBytes>(read4(ifs)),
                           .hash = GitHash(GitHash::decodeBinaryHash(
                               readString(ifs, GitHash::BINARY_HASH_SIZE))),
                           .flags = read2(ifs),
                           .objectName = readStringUntilZero(ifs)});
            int endOfTheEntry = ifs.tellp();
            int entrySize = endOfTheEntry - beginningOfTheEntry;
            int numberOfBytesToPad =
                (8 * std::ceil(entrySize / 8.0)) - entrySize;
            padByByteBoundary(ifs, numberOfBytesToPad);
        }
        return res;
    }
    else {
        GENERATE_EXCEPTION("Wrong number of indices: {}", numberOfIndices);
    }
}
} // namespace Git