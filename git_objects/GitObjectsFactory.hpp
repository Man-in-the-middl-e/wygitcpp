#pragma once

#include "GitObject.hpp"
#include "GitRepository.hpp"

namespace Git {
class GitObjectFactory {
  public:
    static std::unique_ptr<GitObject> create(const std::string& format,
                                             const GitRepository& repo,
                                             const ObjectData& data);

    static std::unique_ptr<GitObject> read(const GitRepository& repo,
                                           const GitHash& sha1);

  private:
    template <class T>
        requires std::is_base_of_v<GitObject, T>
    static std::unique_ptr<GitObject> createObject(const GitRepository& repo,
                                                   const ObjectData& data)
    {
        auto object = std::make_unique<T>(repo, data);
        object->deserialize(data);
        return object;
    }
};
}; // namespace Git

using GitObjectFactory = Git::GitObjectFactory;