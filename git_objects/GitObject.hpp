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
    static GitHash write(GitObject* gitObject, bool acutallyWrite = true);

    static GitHash findObject(const std::string& name,
                              const std::string& format = "");
    static KeyValuesWithMessage
    parseKeyValuesWithMessage(const std::string& data);

    static std::string resolveReference(const std::filesystem::path& reference);

  public:
    // TODO: avoid unnecessary copies, as object data could be up to few dozens
    // megabytes
    // TODO: add new way of construction objcest, mb don't use ObjectData at all
    virtual ObjectData serialize() = 0;
    virtual void deserialize(const ObjectData& data) = 0;
    virtual std::string format() const = 0;

    virtual ~GitObject();

  protected:
    GitObject(const ObjectData& data) : m_data(data) {}

  protected:
    ObjectData m_data;
};

class GitCommit : public GitObject {
  public:
    // TODO: make constructors private, the only proper way to create objects is
    // through factory
    GitCommit(const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;
    std::string format() const override;

    const CommitMessage& commitMessage() const;

  private:
    CommitMessage m_commitMessage;
};

class GitTree : public GitObject {
  public:
    GitTree(const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;
    std::string format() const override;

    const std::vector<GitTreeLeaf> tree() const;

  private:
    std::vector<GitTreeLeaf> parseGitTree(const std::string& data);

  private:
    std::vector<GitTreeLeaf> m_tree;
};

class GitTag : public GitObject {
  public:
    GitTag(const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;
    std::string format() const override;

    const TagMessage& tagMessage() const;

  private:
    TagMessage m_tagMessage;
};

class GitBlob : public GitObject {
  public:
    GitBlob(const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;
    std::string format() const override;

    const ObjectData& blob() const;
};

}; // namespace Git

using GitObject = Git::GitObject;
using GitCommit = Git::GitCommit;
using GitTree = Git::GitTree;
using GitTag = Git::GitTag;
using GitBlob = Git::GitBlob;
using ObjectData = Git::ObjectData;