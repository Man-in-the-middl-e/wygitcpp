#pragma once

#include <bitset>
#include <string>

namespace Git {

class GitHash {
  private:
    static constexpr uint8_t STRING_HASH_SIZE = 40;

  public:
    static constexpr uint8_t BINARY_HASH_SIZE = 20;

  public:
    explicit GitHash(const std::string& hash);

    // Converts 20 bytes binary SHA1 hash to 40 byte readable string
    // representation
    static GitHash decodeBinaryHash(const std::string& data);

    // Converts 40 bytes string representation of SHA1 hash to 20 bytes binary
    // representation
    static std::string encodeStringHash(const GitHash& hash);

    // First two bytes of GitHash are used as directory name for GitObject
    // the rest as file name
    std::string directoryName() const;
    std::string fileName() const;

    const std::string& data() const;
    std::string& data();

  private:
    std::string m_data;
};

std::ostream& operator<<(std::ostream& stream, GitHash hash);
}; // namespace Git

using GitHash = Git::GitHash;