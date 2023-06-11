#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "../git_objects/GitRepository.hpp"
#include "../utilities/SHA1.hpp"

namespace Git {

class ObjectData {
  public:
    ObjectData() = default;
    ObjectData(const std::string& data) : m_data(data) {}
    const std::string& data() const { return m_data; }

  private:
    std::string m_data;
};

struct CommitMessage {
    std::string tree;
    std::string parent;
    std::string author;
    std::string committer;
    std::string gpgsig;
    std::string message;
};

struct TagMessage {
    std::string object;
    std::string type;
    std::string tag;
    std::string tagger;
    std::string gpgsig;
    std::string message;
};

using KeyValuesWithMessage = std::unordered_map<std::string, std::string>;

struct GitTreeLeaf {
    std::string fileMode;
    std::filesystem::path filePath;
    GitHash hash;
};

class GitObject;
class GitObject {
  public:
    static GitHash write(GitObject* gitObject, bool actuallyWrite = true);

    static GitHash findObject(const std::string& name,
                              const std::string& format = "");
    static KeyValuesWithMessage
    parseKeyValuesWithMessage(const std::string& data);

    static std::string resolveReference(const std::filesystem::path& reference);

  public:
    virtual ObjectData serialize() = 0;
    virtual void deserialize(const ObjectData& data) = 0;
    virtual std::string format() const = 0;
    virtual ~GitObject();
};

class GitCommit : public GitObject {
  public:
    GitCommit(const CommitMessage& commitMessage);
    GitCommit() = default;

    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;

    std::string format() const override;

    const CommitMessage& commitMessage() const;

  private:
    CommitMessage m_commitMessage;
};

class GitTree : public GitObject {
  public:
    GitTree() = default;
    GitTree(const std::vector<GitTreeLeaf>& leaves);

    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;
    std::string format() const override;

    const std::vector<GitTreeLeaf>& tree() const;

  public:
    static std::string fileMode(const std::filesystem::directory_entry& entry,
                                const std::string& format);

  private:
    std::vector<GitTreeLeaf> parseGitTree(const std::string& data);

  private:
    std::vector<GitTreeLeaf> m_tree;
};

class GitTag : public GitObject {
  public:
    GitTag(const TagMessage& tagMessage);
    GitTag() = default;

    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;
    std::string format() const override;

    const TagMessage& tagMessage() const;

  private:
    TagMessage m_tagMessage;
};

class GitBlob : public GitObject {
  public:
    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;
    std::string format() const override;

  private:
    ObjectData m_blob;
};

}; // namespace Git

using GitObject = Git::GitObject;
using GitCommit = Git::GitCommit;
using GitTree = Git::GitTree;
using GitTag = Git::GitTag;
using GitBlob = Git::GitBlob;
using ObjectData = Git::ObjectData;
using GitTreeLeaf = Git::GitTreeLeaf;
using TagMessage = Git::TagMessage;