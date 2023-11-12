#pragma once

#include "GitObject.hpp"
#include "GitRepository.hpp"

namespace Git {
class GitObjectFactory {
  public:
    static std::unique_ptr<GitObject> create(const std::string& format,
                                             const ObjectData& data);

    static std::unique_ptr<GitObject> read(const GitHash& sha1);

  private:
    template <class T>
        requires std::is_base_of_v<GitObject, T>
    static std::unique_ptr<GitObject> createObject(const ObjectData& data)
    {
        // TODO: deserialize should verify that object data is valid
        //       so it wouldn't be possible, for example, to create 
        //       a blob with commit data and vice versa. 
        auto object = std::make_unique<T>();
        object->deserialize(data);
        return object;
    }
};
}; // namespace Git

using GitObjectFactory = Git::GitObjectFactory;