#pragma once

#include "GitHash.hpp"
#include <cstdint>
#include <vector>

namespace Git {

struct IndexHeader {
    uint32_t signature;
    uint32_t versionNumber;
    uint32_t numberOfIndexEntries;
};

struct IndexEntry {
    // The last time a file's metadata changed.  This is a tuple (seconds,
    // nanoseconds)
    uint64_t ctime;
    // The last time a file's data changed.  This is a tuple (seconds,
    // nanoseconds)
    uint64_t mtime;
    // The ID of device containing this file
    uint32_t dev;
    // The file's inode number
    uint32_t ino;
    // The object type, either b1000 (regular), b1010 (symlink), b1110
    // (gitlink).
    // TODO: check if the
    uint32_t mode;
    // User ID of owner//
    uint32_t uid;
    // Group ID of ownner (according to stat 2.  Isn'th)
    uint32_t gid;
    // Size of this object, in bytes
    uint32_t fsize;
    // The object's hash as a hex string
    GitHash hash;
    uint16_t flags;
    std::string objectName;
};

class GitIndex {
  public:
    static std::vector<IndexEntry> parse(const std::string& indexFilePath);

  private:
    GitIndex();
    IndexHeader m_header;
    std::vector<IndexEntry> m_entries;
};
}; // namespace Git

using GitIndex = Git::GitIndex;