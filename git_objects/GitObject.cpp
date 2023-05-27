#include "GitObject.hpp"

#include <fstream>
#include <memory>

#include "../utilities/Zlib.hpp"

namespace Git {

GitRepository GitObject::repository() const { return m_repository; }

GitObject::~GitObject() {}

GitHash GitObject::write(GitObject* gitObject, bool acutallyWrite)
{
    auto objectData = gitObject->serialize();
    auto fileContent = gitObject->format() + " " +
                       std::to_string(objectData.data().size()) + '\0' +
                       objectData.data();

    auto fileHash = SHA1::computeHash(fileContent);

    if (acutallyWrite) {
        auto objectFile = GitRepository::repoFile(
            gitObject->repository(), GitRepository::CreateDir::YES, "objects",
            fileHash.directoryName(), fileHash.fileName());
        Zlib::compress(objectFile, fileContent);
    }
    return fileHash;
}

std::filesystem::path GitObject::findObject(const GitRepository& repo,
                                            const std::string& name,
                                            const std::string& format,
                                            bool follow)
{
    return name;
}

GitCommit::GitCommit(const GitRepository& repository, const ObjectData& data)
    : GitObject(repository, data)
{
}

ObjectData GitCommit::serialize() { return {}; }

void GitCommit::deserialize(const ObjectData& data) {}

std::string GitCommit::format() const { return "commit"; }

GitTree::GitTree(const GitRepository& repository, const ObjectData& data)
    : GitObject(repository, data)
{
}

ObjectData GitTree::serialize() { return {}; }

void GitTree::deserialize(const ObjectData& data) {}

std::string GitTree::format() const { return "tree"; }

GitTag::GitTag(const GitRepository& repository, const ObjectData& data)
    : GitObject(repository, data)
{
}

ObjectData GitTag::serialize() { return {}; }

void GitTag::deserialize(const ObjectData& data) {}

std::string GitTag::format() const { return "tag"; }

GitBlob::GitBlob(const GitRepository& repository, const ObjectData& data)
    : GitObject(repository, data)
{
}

ObjectData GitBlob::serialize() { return m_data; }

void GitBlob::deserialize(const ObjectData& data) { m_data = data; }

std::string GitBlob::format() const { return "blob"; }
} // namespace Git
