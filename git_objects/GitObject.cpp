#include "GitObject.hpp"
#include "../utilities/Zlib.hpp"
#include "GitObjectsFactory.hpp"

#include <assert.h>
#include <fstream>
#include <regex>

namespace {
std::vector<GitHash> resolveObject(const GitRepository& repo,
                                   const std::string& name)
{
    if (name.empty()) {
        return {};
    }
    if (name == "HEAD") {
        // TODO: smells very badly, way too many conversions
        return {GitHash(GitObject::resolveReference(
            GitRepository::repoFile(GitRepository::findRoot(), "HEAD")))};
    }

    std::regex shaSignature("[0-9A-Fa-f]{4,40}");
    std::smatch cm;

    std::vector<GitHash> candiates;
    if (auto matched = std::regex_match(name, cm, shaSignature); matched) {
        auto sha = cm.str();

        if (sha.size() == 40) {
            return {GitHash(sha)};
        }

        auto hashPrefix = name.substr(0, 2);
        auto objectPath = GitRepository::repoDir(repo, "objects", hashPrefix);

        if (!objectPath.empty()) {
            auto beginningOfHash = name.substr(2);
            for (std::filesystem::directory_entry dirEntry :
                 std::filesystem::directory_iterator{objectPath}) {
                auto hashSuffix = dirEntry.path().filename().string();
                if (dirEntry.is_regular_file() &&
                    hashSuffix.starts_with(beginningOfHash)) {
                    candiates.push_back(GitHash(hashPrefix + hashSuffix));
                }
            }
        }
    }
    else {
        // if name is not hash, than it's tag or branch
        auto branchesPath = GitRepository::repoPath(repo, "branches");
        for (std::filesystem::directory_entry dirEntry :
             std::filesystem::directory_iterator{branchesPath}) {
            if (dirEntry.path().filename().string() == name) {
                candiates.push_back(
                    GitHash(GitObject::resolveReference(branchesPath / name)));
            }
        }

        auto tagsPath = GitRepository::repoPath(repo, "refs", "tags");
        if (candiates.empty()) {
            for (std::filesystem::directory_entry dirEntry :
                 std::filesystem::directory_iterator{tagsPath}) {
                if (dirEntry.path().filename().string() == name) {
                    candiates.push_back(
                        GitHash(GitObject::resolveReference(tagsPath / name)));
                }
            }
        }
    }
    return candiates;
}
}; // namespace

namespace Git {

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
            GitRepository::findRoot(), GitRepository::CreateDir::YES, "objects",
            fileHash.directoryName(), fileHash.fileName());
        Zlib::compress(objectFile, fileContent);
    }
    return fileHash;
}

GitHash GitObject::findObject(const std::string& name, const std::string& fmt)
{
    auto repo = GitRepository::findRoot();
    auto shas = resolveObject(repo, name);
    if (shas.empty()) {
        GENERATE_EXCEPTION("No such reference: {}", name);
    }

    if (shas.size() > 1) {
        GENERATE_EXCEPTION("Ambiguous reference for {0}:", name);
    }

    auto sha = shas[0];

    if (fmt.empty()) {
        return sha;
    }

    while (true) {
        auto object = GitObjectFactory::read(sha);

        if (object->format() == fmt) {
            return sha;
        }

        if (object->format() == "tag") {
            auto tag = static_cast<GitTag*>(object.get());
            sha = GitHash(tag->tagMessage().object);
        }
        else if (object->format() == "commit") {
            auto commit = static_cast<GitCommit*>(object.get());
            sha = GitHash(commit->commitMessage().tree);
        }
        else {
            GENERATE_EXCEPTION("Invalid object format: {}", fmt);
        }
    }
}
/*
    Parse files that conatins key value paris on each line, separated by space,
    with optional gpgsig, and message that is separated by new line.
    Example:
        tree 29ff16c9c14e2652b22f8b78bb08a5a07930c147
        parent 206941306e8a8af65b66eaaaea388a7ae24d49a0
        author Thibault Polge <thibault@thb.lt> 1527025023 +0200
        committer Thibault Polge <thibault@thb.lt> 1527025044 +0200
        gpgsig -----BEGIN PGP SIGNATURE-----

        iQIzBAABCAAdFiEExwXquOM8bWb4Q2zVGxM2FxoLkGQFAlsEjZQACgkQGxM2FxoL
        kGQdcBAAqPP+ln4nGDd2gETXjvOpOxLzIMEw4A9gU6CzWzm+oB8mEIKyaH0UFIPh
        rNUZ1j7/ZGFNeBDtT55LPdPIQw4KKlcf6kC8MPWP3qSu3xHqx12C5zyai2duFZUU
        wqOt9iCFCscFQYqKs3xsHI+ncQb+PGjVZA8+jPw7nrPIkeSXQV2aZb1E68wa2YIL
        3eYgTUKz34cB6tAq9YwHnZpyPx8UJCZGkshpJmgtZ3mCbtQaO17LoihnqPn4UOMr
        V75R/7FjSuPLS8NaZF4wfi52btXMSxO/u7GuoJkzJscP3p4qtwe6Rl9dc1XC8P7k
        NIbGZ5Yg5cEPcfmhgXFOhQZkD0yxcJqBUcoFpnp2vu5XJl2E5I/quIyVxUXi6O6c
        /obspcvace4wy8uO0bdVhc4nJ+Rla4InVSJaUaBeiHTW8kReSFYyMmDCzLjGIu1q
        doU61OM3Zv1ptsLu3gUE6GU27iWYj2RWN3e3HE4Sbd89IFwLXNdSuM0ifDLZk7AQ
        WBhRhipCCgZhkj9g2NEk7jRVslti1NdN5zoQLaJNqSwO1MtxTmJ15Ksk3QP6kfLB
        Q52UWybBzpaP9HEd4XnR+HuQ4k2K0ns2KgNImsNvIyFwbpMUyUWLMPimaV1DWUXo
        5SBjDB/V/W2JBFR+XKHFJeFwYhj7DD/ocsGr4ZMx/lgc8rjIBkI=
        =lgTX
        -----END PGP SIGNATURE-----

        Create first draft
*/
KeyValuesWithMessage
GitObject::parseKeyValuesWithMessage(const std::string& data)
{
    KeyValuesWithMessage objectData;

    size_t start = 0;
    auto maxSize = data.size();

    while (start < maxSize) {
        // message is separated by blank line from all other data.
        if (data[start] == '\n') {
            objectData["message"] = data.substr(start + 1);
            break;
        }

        auto keyEnds = data.find(' ', start);
        auto valueEnds = data.find('\n', keyEnds);

        auto key = data.substr(start, keyEnds - start);
        if (key == "gpgsig") {
            auto gpgsigEnds = data.find("\n\n");
            objectData[key] += data.substr(start + key.size(),
                                           gpgsigEnds - start - key.size());
            // point to '\n', so the message could be parse properly
            start = gpgsigEnds + 1;
            continue;
        }

        auto value = data.substr(keyEnds + 1, valueEnds - keyEnds - 1);
        objectData[key] = value;
        start = valueEnds + 1;
    }
    return objectData;
}

std::string
GitObject::resolveReference(const std::filesystem::path& refereceDir)
{
    auto referenceContet = Utilities::readFile(refereceDir);
    if (referenceContet.back() == '\n') {
        referenceContet.erase(referenceContet.end() - 1);
    }
    if (referenceContet.starts_with("ref: ")) {
        auto indirectReference = referenceContet.substr(5);
        auto fullPathToIndirectReference = GitRepository::repoFile(
            GitRepository::findRoot(), indirectReference);
        return resolveReference(fullPathToIndirectReference);
    }
    else {
        return referenceContet;
    }
}

GitCommit::GitCommit(const ObjectData& data) : GitObject(data) {}

ObjectData GitCommit::serialize()
{
    std::ostringstream oss;

    oss << "tree"
        << " " << m_commitMessage.tree << std::endl;
    if (!m_commitMessage.parent.empty()) {
        oss << "parent"
            << " " << m_commitMessage.parent << std::endl;
    }
    oss << "author"
        << " " << m_commitMessage.author << std::endl;
    oss << "committer"
        << " " << m_commitMessage.committer << std::endl;

    if (!m_commitMessage.gpgsig.empty()) {
        oss << "gpgsig"
            << " " << m_commitMessage.gpgsig << std::endl;
    }
    oss << std::endl;
    oss << m_commitMessage.message;

    return ObjectData(oss.str());
}

void GitCommit::deserialize(const ObjectData& data)
{
    auto commitMessage = GitObject::parseKeyValuesWithMessage(data.data());
    // NOTE: use [], so if the element is not present empty string will be returend:)
    m_commitMessage = {.tree = commitMessage["tree"],
                       .parent = commitMessage["parent"],
                       .author = commitMessage["author"],
                       .committer = commitMessage["committer"],
                       .gpgsig = commitMessage["gpgsig"],
                       .message = commitMessage["message"]};
}

std::string GitCommit::format() const { return "commit"; }

const CommitMessage& GitCommit::commitMessage() const
{
    return m_commitMessage;
}

GitTree::GitTree(const ObjectData& data) : GitObject(data) {}

ObjectData GitTree::serialize()
{
    std::string data;

    for (const auto& leaf : m_tree) {
        data += leaf.fileMode;
        data += ' ';
        data += leaf.filePath;
        data += '\0';
        data += GitHash::encodeStringHash(leaf.hash);
    }
    return ObjectData(data);
}

std::vector<GitTreeLeaf> GitTree::parseGitTree(const std::string& data)
{
    std::vector<GitTreeLeaf> tree;

    auto maxPos = data.size();
    size_t start = 0;

    while (start < maxPos) {
        auto fileModeEnds = data.find(' ', start);
        auto fileMode = data.substr(start, fileModeEnds - start);
        assert(fileMode.size() == 5 || fileMode.size() == 6);

        auto pathEnds = data.find('\0', fileModeEnds);
        auto path = data.substr(fileModeEnds + 1, pathEnds - fileModeEnds - 1);

        auto sha = GitHash::decodeBinaryHash(
            data.substr(pathEnds + 1, GitHash::BINARY_HASH_SIZE));

        start = pathEnds + GitHash::BINARY_HASH_SIZE + 1;
        tree.push_back(
            {.fileMode = fileMode, .filePath = path, .hash = GitHash(sha)});
    }
    return tree;
}

void GitTree::deserialize(const ObjectData& data)
{
    m_tree = parseGitTree(data.data());
}

std::string GitTree::format() const { return "tree"; }

const std::vector<GitTreeLeaf> GitTree::tree() const { return m_tree; }

GitTag::GitTag(const ObjectData& data) : GitObject(data) {}

ObjectData GitTag::serialize()
{
    std::ostringstream oss;

    oss << "object"
        << " " << m_tagMessage.object << std::endl;
    oss << "type"
        << " " << m_tagMessage.type << std::endl;
    oss << "tag"
        << " " << m_tagMessage.tag << std::endl;
    oss << "tagger"
        << " " << m_tagMessage.tagger << std::endl;

    if (!m_tagMessage.gpgsig.empty()) {
        oss << "gpgsig"
            << " " << m_tagMessage.gpgsig << std::endl;
    }
    oss << std::endl;
    oss << m_tagMessage.message;

    return ObjectData(oss.str());
}

void GitTag::deserialize(const ObjectData& data)
{
    auto tagMessage = GitObject::parseKeyValuesWithMessage(data.data());
    m_tagMessage = {.object = tagMessage["object"],
                    .type = tagMessage["type"],
                    .tag = tagMessage["tag"],
                    .tagger = tagMessage["tagger"],
                    .gpgsig = tagMessage["gpgsig"],
                    .message = tagMessage["message"]};
}

std::string GitTag::format() const { return "tag"; }

const TagMessage& GitTag::tagMessage() const { return m_tagMessage; }

GitBlob::GitBlob(const ObjectData& data) : GitObject(data) {}

ObjectData GitBlob::serialize() { return m_data; }

void GitBlob::deserialize(const ObjectData& data) { m_data = data; }

std::string GitBlob::format() const { return "blob"; }
} // namespace Git
