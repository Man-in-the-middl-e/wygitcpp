#pragma once

#include <assert.h>
#include <fstream>
#include <variant>

#include "../utilities/Common.hpp"
#include "GitObject.hpp"

namespace Git {
namespace Fs = std::filesystem;

enum class HeadType : bool { REF, HASH };

class GitHash;
class GitRepository {
  public:
    enum class CreateDir { YES = 0, NO = 1 };
    using Fpath = std::filesystem::path;

  public:
    static GitRepository initialize(const Fpath& path);
    static GitRepository create(const Fpath& path);
    static GitRepository findRoot(const Fpath& path = ".");

    static void setHEAD(const std::string& value);
    static void setHEAD(const GitHash& hash);
    static void commitToBranch(const GitHash& commitHash);

    static std::string HEAD(HeadType type = HeadType::HASH);
    static Fpath pathToHead();

  public:
    template <class... T>
    static Fpath repoPath(const GitRepository& repo, T&&... path)
    {
        Fpath repoPath = (repo.gitDir() / ... / path);
        return repoPath;
    }

    template <class... T>
    static Fpath repoDir(const GitRepository& repo, CreateDir mkdir,
                         T&&... path)
    {
        auto repositoryDir = repoPath(repo, std::forward<T>(path)...);
        if (Fs::exists(repositoryDir)) {
            if (Fs::is_directory(repositoryDir)) {
                return repositoryDir;
            }
            else {
                GENERATE_EXCEPTION("Not a directory: {}",
                                   repositoryDir.string());
            }
        }

        if (mkdir == CreateDir::YES) {
            Fs::create_directories(repositoryDir);
            return repositoryDir;
        }
        return {};
    }

    template <class... T>
    static Fpath repoDir(const GitRepository& repo, T&&... path)
    {
        return repoDir(repo, CreateDir::NO, std::forward<T>(path)...);
    }

    template <class... T>
    static Fpath repoFile(const GitRepository& repo, CreateDir mkdir,
                          T&&... path)
    {
        // TODO: make it less complex
        auto filePath = (Fpath("") / ... / path);
        auto parentFilePath = filePath.parent_path();
        if (auto dir = repoDir(repo, mkdir, parentFilePath); !dir.empty()) {
            return repoPath(repo, filePath);
        }
        return {};
    }

    template <class... T>
    static Fpath repoFile(const GitRepository& repo, T&&... path)
    {
        return repoFile(repo, CreateDir::NO, std::forward<T>(path)...);
    }

    const Fpath& gitDir() const;
    const Fpath& workTree() const;

  private:
    GitRepository(const Fpath& workTree, const Fpath& gitDir);

  private:
    Fpath m_workTree;
    Fpath m_gitDir;
};
}; // namespace Git

using GitRepository = Git::GitRepository;
using HeadType = Git::HeadType;
