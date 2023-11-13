#pragma once

#include <bitset>
#include <string>

namespace Git {

class BinaryHash {
  public:
    static constexpr uint8_t SIZE = 20;

  public:
    explicit BinaryHash(const std::string& data);
    const std::string& data() const;

  private:
    std::string m_data;
};

class GitHash {
  public:
    explicit GitHash(const std::string& hash);
    explicit GitHash(const BinaryHash& hash);

    // Converts 40 bytes string representation of SHA1 hash to 20 bytes binary
    // representation
    static BinaryHash convertToBinary(const GitHash& hash);

    const std::string& data() const;
    std::string& data();

  private:
    std::string m_data;

  private:
    static constexpr uint8_t READABLE_HASH_SIZE = 40;
};

std::ostream& operator<<(std::ostream& stream, GitHash hash);
bool operator==(const GitHash& lhs, const GitHash& rhs);
}; // namespace Git

using GitHash = Git::GitHash;