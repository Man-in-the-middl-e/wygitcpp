#pragma once

#include <memory>

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
    std::string messaage;
};

struct GitTreeLeaf {
    std::string fileMode;
    std::filesystem::path filePath;
    GitHash hash;
};

class GitObject;
class GitObject {
  public:
    static GitHash write(GitObject* gitObject, bool acutallyWrite = true);

    static std::filesystem::path findObject(const GitRepository& repo,
                                            const std::string& name,
                                            const std::string& format = "",
                                            bool follow = true);

  public:
    // TODO: avoid unnecessary copies, as object data could be up to few dozens
    // megabytes
    virtual ObjectData serialize() = 0;
    virtual void deserialize(const ObjectData& data) = 0;
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
    void deserialize(const ObjectData& data) override;
    std::string format() const override;

    const CommitMessage& message() const;

  private:
   /*
     Message example, gpgsig is optional: 
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
    CommitMessage parseCommitMessage(const std::string& data);

  private:
    CommitMessage m_commitMessage;
};

class GitTree : public GitObject {
  public:
    GitTree(const GitRepository& repository, const ObjectData& data);

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
    GitTag(const GitRepository& repository, const ObjectData& data);

    ObjectData serialize() override;
    void deserialize(const ObjectData& data) override;
    std::string format() const override;
};

class GitBlob : public GitObject {
  public:
    GitBlob(const GitRepository& repository, const ObjectData& data);

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