#pragma once

#include <string>

namespace Utilities {
class SHA1 {
  private:
    static constexpr uint8_t HASH_SIZE_BYTES = 20;
    static constexpr uint8_t DIGEST_SIZE_WORD = 5;

  public:
    SHA1(const std::string& data);
    const std::string& toString() const;

  private:
    std::string generateSHA(const std::string& data) const;

    std::string SHA1ToString(const std::string& hash) const;

  private:
    std::string m_hashData;
};
};

using SHA1 = Utilities::SHA1;