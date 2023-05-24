#pragma once

#include <memory>

#include "../GitRepository.hpp"
#include "../utilities/SHA1.hpp"

namespace Git {

class ObjectData {
  public:
    ObjectData() = default;
    ObjectData(const std::string& data) : m_data(data) {}
    const std::string& data() { return m_data; }

  private:
    std::string m_data;
};

class GitObject;
class GitObject {
  public:
    static std::unique_ptr<GitObject> read(const GitRepository& repo,
                                           const std::string& sha1);

    static SHA1 write(GitObject* gitObject, bool acutallyWrite = true);

    static std::filesystem::path findObject(const GitRepository& repo,
                                            const std::string& name,
                                            const std::string& format,
                                            bool follow = true);

    static std::unique_ptr<GitObject> create(const std::string& format,
                                             const GitRepository& repo,
                                             const ObjectData& data);

  public:
    // TODO: avoid unnecessary copies, as object data could be up to few dozens MB
    virtual ObjectData serialize() = 0;
    virtual void deserialize(ObjectData& data) = 0;
    virtual std::string format() const = 0;

    GitRepository repository() const;

    virtual ~GitObject();

  protected:
    GitObject(const GitRepository& repository, const ObjectData& data)
        : m_repository(repository), m_data(data)
    {
    }

  protected:
    GitRepository m_repository;
    ObjectData m_data;
};

class GitCommit : public GitObject {
  public:
    GitCommit(const GitRepository& repository, const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(ObjectData& data) override;
    std::string format() const override;
};

class GitTree : public GitObject {
  public:
    GitTree(const GitRepository& repository, const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(ObjectData& data) override;
    std::string format() const override;
};

class GitTag : public GitObject {
  public:
    GitTag(const GitRepository& repository, const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(ObjectData& data) override;
    std::string format() const override;
};

class GitBlob : public GitObject {
  public:
    GitBlob(const GitRepository& repository, const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(ObjectData& data) override;
    std::string format() const override;
};

}; // namespace Git
