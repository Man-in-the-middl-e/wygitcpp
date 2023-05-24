#pragma once

#include <string>

namespace Git {

class GitHash {
  public:
    explicit GitHash(const std::string& hash);

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