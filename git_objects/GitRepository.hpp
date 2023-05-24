#pragma once

#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "../utilities/Common.hpp"

namespace Git {
namespace Fs = std::filesystem;
namespace ConfigurationParser = boost::property_tree;

class GitRepository {
  public:
    enum class CreateDir { YES = 0, NO = 1 };
    using Fpath = std::filesystem::path;

  public:
    static GitRepository initialize(const Fpath& path);
    static GitRepository create(const Fpath& path, bool force = false);
    static std::optional<GitRepository>
    findRootGitRepository(const GitRepository::Fpath& path = ".",
                          bool required = true);

  public:
    template <class... T>
    static Fpath repoPath(const GitRepository& repo, T&&... path)
    {
        std::filesystem::path repoPath = (repo.gitDir() / ... / path);
        return repoPath;
    }

    template <class... T>
    static Fpath repoDir(const GitRepository& repo, CreateDir mkdir,
                         T&&... path)
    {
        auto repositoryPath = repoPath(repo, std::forward<T>(path)...);
        if (std::filesystem::exists(repositoryPath)) {
            if (std::filesystem::is_directory(repositoryPath)) {
                return repositoryPath;
            }
            else {
                throw std::runtime_error(fmt::format("Not a directory: {}",
                                                     repositoryPath.string()));
            }
        }

        if (mkdir == CreateDir::YES) {
            std::filesystem::create_directories(repositoryPath);
            return repositoryPath;
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

    static void writeToFile(const Fpath& path, const std::string& content)
    {
        std::fstream currentFile(path.string(),
                                 std::fstream::out | std::fstream::trunc);
        if (currentFile.is_open()) {
            currentFile << content;
            return;
        }
        GENERATE_EXCEPTION("Couldn't open file: {}", path.string());
    }

    Fpath gitDir() const;

  private:
    GitRepository(const Fpath& workTree, const Fpath& gitDir,
                  const boost::property_tree::ptree& config);

  private:
    Fpath m_workTree;
    Fpath m_gitDir;
    ConfigurationParser::ptree m_configuration;
};
}; // namespace Git

using GitRepository = Git::GitRepository;
