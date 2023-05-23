#include "GitObject.hpp"
#include "../utilities/Zlib.hpp"

#include <fstream>
#include <memory>

namespace Git {

GitRepository GitObject::repository() const { return m_repository; }

GitObject::~GitObject() {}

std::unique_ptr<GitObject> GitObject::create(const std::string& format,
                                             const GitRepository& repo,
                                             const ObjectData& objectData)
{
    if (format == "commit") {
        return std::make_unique<GitCommit>(repo, objectData);
    }
    else if (format == "tree") {
        return std::make_unique<GitTree>(repo, objectData);
    }
    else if (format == "tag") {
        return std::make_unique<GitTag>(repo, objectData);
    }
    else if (format == "blob") {
        return std::make_unique<GitBlob>(repo, objectData);
    }
    return nullptr;
}

std::unique_ptr<GitObject> GitObject::read(const GitRepository& repo,
                                           const std::string& sha)
{
    // First 2 char of SHA1 hash is used as directory name
    // rest as file name
    auto directoryName = sha.substr(0, 2);
    auto fileName = sha.substr(2);
    auto path =
        GitRepository::repoFile(repo, "objects", directoryName, fileName);
        
    auto objectContent = Zlib::decompressFile(path);
    /*
        |object type|| ||size||0|
        |content ...|
    */
    auto formatEnds = objectContent.find_first_of(' ');
    auto format = objectContent.substr(0, formatEnds);

    auto sizeEnds = objectContent.find_first_of('\0', formatEnds);
    auto size = std::stoi(objectContent.substr(formatEnds, sizeEnds));

    if (size != objectContent.size() - sizeEnds - 1) {
        GENERATE_EXCEPTION("Malformed object: {}", sha);
    }

    auto objectData = ObjectData(objectContent.substr(sizeEnds + 1));
    if (auto object = create(format, repo, objectData); !object) {
        GENERATE_EXCEPTION("Unknown type {} for object {}", format, sha);
    } else {
        return object;
    }
}

SHA1 GitObject::write(GitObject* gitObject, bool acutallyWrite)
{
    auto objectData = gitObject->serialize();
    auto fileContent = gitObject->format() + " " +
                       std::to_string(objectData.data().size()) + '\0' +
                       objectData.data();

    SHA1 fileHash(fileContent);
    auto repositoryDir = fileHash.toString().substr(0, 2);
    auto repositoryName = fileHash.toString().substr(2);

    if (acutallyWrite) {
        auto objectFile = GitRepository::repoFile(
            gitObject->repository(), GitRepository::CreateDir::YES, "objects",
            repositoryDir, repositoryName);
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

void GitCommit::deserialize(ObjectData& data) {}

std::string GitCommit::format() const { return "commit"; }

GitTree::GitTree(const GitRepository& repository, const ObjectData& data)
    : GitObject(repository, data)
{
}

ObjectData GitTree::serialize() { return {}; }

void GitTree::deserialize(ObjectData& data) {}

std::string GitTree::format() const { return "tree"; }

GitTag::GitTag(const GitRepository& repository, const ObjectData& data)
    : GitObject(repository, data)
{
}

ObjectData GitTag::serialize() { return {}; }

void GitTag::deserialize(ObjectData& data) {}

std::string GitTag::format() const { return "tag"; }

GitBlob::GitBlob(const GitRepository& repository, const ObjectData& data)
    : GitObject(repository, data)
{
}

ObjectData GitBlob::serialize() { return m_data; }

void GitBlob::deserialize(ObjectData& data) { m_data = data; }

std::string GitBlob::format() const { return "blob"; }
} // namespace Git
