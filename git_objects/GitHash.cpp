#include "GitHash.hpp"

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Git {
std::string GitHash::encodeStringHash(const GitHash& hash)
{
    std::string res;
    res.reserve(BINARY_HASH_SIZE);
    auto& data = hash.data();
    for (int blobIndex = 0; blobIndex < data.size(); blobIndex += 2) {
        uint8_t blob = std::stoi(data.substr(blobIndex, 2), nullptr, 16);
        res.push_back(blob);
    }
    return res;
}

GitHash GitHash::decodeBinaryHash(const std::string& data)
{
    // TODO: improve binary data representation, replace std::string with
    // something more descriptive and type safe
    assert(data.size() == BINARY_HASH_SIZE);
    std::ostringstream oss;
    oss << std::hex;
    for (int blobIndex = 0; blobIndex < data.size(); blobIndex += 1) {
        oss << std::setw(2) << std::setfill('0')
            << ((uint16_t)data[blobIndex] & 0xff);
    }
    return GitHash(oss.str());
}

GitHash::GitHash(const std::string& hash) : m_data(hash)
{
    assert(m_data.size() == STRING_HASH_SIZE);
}

std::string GitHash::directoryName() const { return m_data.substr(0, 2); }

std::string GitHash::fileName() const { return m_data.substr(2); }

const std::string& GitHash::data() const { return m_data; }
std::string& GitHash::data() { return m_data; }
std::ostream& operator<<(std::ostream& stream, GitHash hash)
{
    stream << hash.data();
    return stream;
}
}; // namespace Git