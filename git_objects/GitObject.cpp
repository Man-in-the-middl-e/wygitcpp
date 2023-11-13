#include "GitObject.hpp"
#include "../utilities/Zlib.hpp"
#include "GitObjectsFactory.hpp"

#include <assert.h>
#include <fstream>
#include <regex>

namespace {
std::vector<GitHash> resolveObject(const std::string& name)
{
    if (name.empty()) {
        return {};
    }
    if (name == "HEAD") {
        return {
            GitHash(GitObject::resolveReference(GitRepository::pathToHead()))};
    }

    std::regex shaSignature("[0-9A-Fa-f]{4,40}");
    std::smatch cm;

    std::vector<GitHash> candidates;
    if (auto matched = std::regex_match(name, cm, shaSignature); matched) {
        auto sha = cm.str();

        if (sha.size() == 40) {
            return {GitHash(sha)};
        }

        auto hashPrefix = name.substr(0, 2);
        auto objectPath = GitRepository::repoDir("objects", hashPrefix);

        if (!objectPath.empty()) {
            auto beginningOfHash = name.substr(2);
            for (std::filesystem::directory_entry dirEntry :
                 std::filesystem::directory_iterator{objectPath}) {
                auto hashSuffix = dirEntry.path().filename().string();
                if (dirEntry.is_regular_file() &&
                    hashSuffix.starts_with(beginningOfHash)) {
                    candidates.push_back(GitHash(hashPrefix + hashSuffix));
                }
            }
        }
    }
    else {
        // if name is not hash, than it's tag or branch
        auto branchesPath = GitRepository::repoDir("refs", "heads");
        for (std::filesystem::directory_entry dirEntry :
             std::filesystem::directory_iterator{branchesPath}) {
            if (dirEntry.path().filename().string() == name) {
                candidates.push_back(
                    GitHash(GitObject::resolveReference(branchesPath / name)));
            }
        }

        auto tagsPath = GitRepository::repoDir("refs", "tags");
        if (candidates.empty()) {
            for (std::filesystem::directory_entry dirEntry :
                 std::filesystem::directory_iterator{tagsPath}) {
                if (dirEntry.path().filename().string() == name) {
                    candidates.push_back(
                        GitHash(GitObject::resolveReference(tagsPath / name)));
                }
            }
        }
    }
    return candidates;
}
}; // namespace

namespace Git {

GitObject::~GitObject() {}

GitHash GitObject::write(GitObject* gitObject, bool actuallyWrite)
{
    auto objectData = gitObject->serialize();
    auto fileContent = gitObject->format() + " " +
                       std::to_string(objectData.data().size()) + '\0' +
                       objectData.data();

    auto fileHash = SHA1::computeHash(fileContent);
    if (actuallyWrite) {
        auto objectFile =
            GitRepository::repoFile(GitRepository::CreateDir::YES, "objects",
                                    Utilities::getObjectDirectory(fileHash),
                                    Utilities::getObjectFileName(fileHash));
        Zlib::compress(objectFile, fileContent);
    }
    return fileHash;
}

GitHash GitObject::findObject(const std::string& name, const std::string& fmt)
{
    auto shas = resolveObject(name);
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
    Parse files that contains key value paris on each line, separated by space,
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
GitObject::resolveReference(const std::filesystem::path& referenceDir,
                            bool dereference)
{
    auto referenceContent = Utilities::readFile(referenceDir);
    if (referenceContent.back() == '\n') {
        referenceContent.erase(referenceContent.end() - 1);
    }
    if (referenceContent.starts_with("ref: ") && dereference) {
        auto indirectReference = referenceContent.substr(5);
        auto fullPathToIndirectReference =
            GitRepository::repoFile(indirectReference);
        return resolveReference(fullPathToIndirectReference);
    }
    else {
        return referenceContent;
    }
}

GitCommit::GitCommit(const CommitMessage& commitMessage)
    : m_commitMessage(commitMessage)
{
}

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
    // NOTE: use [], so if the element is not present empty string will be
    // returned:)
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

GitTree::GitTree(const std::vector<GitTreeLeaf>& leaves) : m_tree(leaves) {}

ObjectData GitTree::serialize()
{
    std::string data;

    for (const auto& leaf : m_tree) {
        data += leaf.fileMode;
        data += ' ';
        data += leaf.filePath;
        data += '\0';
        data += GitHash::convertToBinary(leaf.hash).data();
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

        auto sha = GitHash(
            BinaryHash(data.substr(pathEnds + 1, BinaryHash::SIZE)));

        start = pathEnds + BinaryHash::SIZE + 1;
        tree.push_back({.fileMode = fileMode, .filePath = path, .hash = sha});
    }
    return tree;
}

void GitTree::deserialize(const ObjectData& data)
{
    m_tree = parseGitTree(data.data());
}

std::string GitTree::format() const { return "tree"; }

const std::vector<GitTreeLeaf>& GitTree::tree() const { return m_tree; }

/*
The file mode; one of 100644 for file (blob), 100755 for executable (blob),
040000 for subdirectory (tree), 160000 for submodule (commit),
or 120000 for a blob that specifies the path of a symlink.
Can be one of: 100644, 100755, 040000, 160000, 120000
*/
std::string GitTree::fileMode(const std::filesystem::directory_entry& entry,
                              const std::string& format)
{
    using std::filesystem::perms;
    auto isExecutable = [=](perms filePerms) {
        return (perms::none != (filePerms & perms::owner_exec)) ||
               (perms::none != (filePerms & perms::group_exec)) ||
               (perms::none != (filePerms & perms::others_exec));
    };

    if ((entry.is_regular_file() || entry.is_symlink()) && format == "blob") {
        auto filePerms =
            std::filesystem::status(entry.path().filename()).permissions();
        if (isExecutable(filePerms)) {
            return "100755";
        }
        else if (entry.is_symlink()) {
            return "120000";
        }
        else {
            return "100644";
        }
    }
    else if (entry.is_directory()) {
        if (format == "tree") {
            return "040000";
        }
        else if (format == "commit") {
            return "160000";
        }
    }
    GENERATE_EXCEPTION(
        "Wrong file format or directory entry. Directory: {}, format: {}",
        entry.path().string(), format);
}

GitTag::GitTag(const TagMessage& tagMessage) : m_tagMessage(tagMessage) {}

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
    oss << m_tagMessage.message << std::endl;

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

ObjectData GitBlob::serialize() { return m_blob; }

void GitBlob::deserialize(const ObjectData& data) { m_blob = data; }

std::string GitBlob::format() const { return "blob"; }
} // namespace Git
