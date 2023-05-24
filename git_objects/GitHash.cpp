#include "GitHash.hpp"

#include <assert.h>
#include <iostream>

namespace Git {
GitHash::GitHash(const std::string& hash) : m_data(hash)
{
    assert(m_data.size() == 40);
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