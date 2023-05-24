#include "SHA1.hpp"

#include <boost/uuid/detail/sha1.hpp>
#include <sstream>

namespace Utilities {
GitHash SHA1::computeHash(const std::string& data)
{
    return GitHash(makeHashReadable(generateHash(data)));
}

std::string SHA1::generateHash(const std::string& data)
{
    static constexpr uint8_t HASH_SIZE_BYTES = 20;
    static constexpr uint8_t DIGEST_SIZE_WORD = 5;

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

std::string SHA1::makeHashReadable(const std::string& hash)
{
    std::string res(hash.size() * 2, '\0');
    std::string_view hexDigits = "0123456789abcdef";
    for (size_t i = 0; i < hash.size(); ++i) {
        res[i * 2] = hexDigits[(hash[i] >> 4) & 0xf];
        res[i * 2 + 1] = hexDigits[hash[i] & 0xf];
    }
    return res;
}
}; // namespace Utilities