#include "SHA1.hpp"

#include <boost/uuid/detail/sha1.hpp>
#include <sstream>

namespace Utilities {
SHA1::SHA1(const std::string& data)
    : m_hashData(SHA1ToString(generateSHA(data)))
{
}

const std::string& SHA1::toString() const { return m_hashData; }

std::string SHA1::generateSHA(const std::string& data) const
{
    boost::uuids::detail::sha1 s;
    s.process_bytes(data.c_str(), data.size());
    unsigned int digest[DIGEST_SIZE_WORD];
    s.get_digest(digest);

    std::string hash(HASH_SIZE_BYTES, '\0');
    for (int i = 0; i < DIGEST_SIZE_WORD; ++i) {
        const char* tmp = reinterpret_cast<char*>(digest);
        hash[i * 4] = tmp[i * 4 + 3];
        hash[i * 4 + 1] = tmp[i * 4 + 2];
        hash[i * 4 + 2] = tmp[i * 4 + 1];
        hash[i * 4 + 3] = tmp[i * 4];
    }
    return hash;
}

std::string SHA1::SHA1ToString(const std::string& hash) const
{
    std::string res(hash.size() * 2, '\0');
    std::string_view hexDigits = "0123456789abcdef";
    for (size_t i = 0; i < hash.size(); ++i) {
        res[i * 2] = hexDigits[(hash[i] >> 4) & 0xf];
        res[i * 2 + 1] = hexDigits[hash[i] & 0xf];
    }
    return res;
}
};