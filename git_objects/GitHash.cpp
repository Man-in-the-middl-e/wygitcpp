#include "GitHash.hpp"

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Git {

BinaryHash::BinaryHash(const std::string& hash) : m_data(hash)
{
    assert(hash.size() == BinaryHash::SIZE);
}

const std::string& BinaryHash::data() const { return m_data; }

BinaryHash GitHash::convertToBinary(const GitHash& hash)
{
    std::string res;
    res.reserve(BinaryHash::SIZE);
    auto& data = hash.data();
    for (int blobIndex = 0; blobIndex < data.size(); blobIndex += 2) {
        uint8_t blob = std::stoi(data.substr(blobIndex, 2), nullptr, 16);
        res.push_back(blob);
    }
    return BinaryHash(res);
}

GitHash convertFromBinary(const BinaryHash& hash)
{
    auto& data = hash.data();
    assert(data.size() == BinaryHash::SIZE);
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
    assert(m_data.size() == READABLE_HASH_SIZE);
}

GitHash::GitHash(const BinaryHash& hash)
    : m_data(convertFromBinary(hash).data())
{
}

const std::string& GitHash::data() const { return m_data; }
std::string& GitHash::data() { return m_data; }
std::ostream& operator<<(std::ostream& stream, GitHash hash)
{
    stream << hash.data();
    return stream;
}
bool operator==(const GitHash& lhs, const GitHash& rhs)
{
    return lhs.data() == rhs.data();
}
}; // namespace Git