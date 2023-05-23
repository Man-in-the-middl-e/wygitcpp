#include <assert.h>
#include <boost/property_tree/ini_parser.hpp>
#include <fstream>

#include "GitRepository.hpp"

namespace Git {
void writeDefaultConfiguration(const GitRepository::Fpath& configFilePath)
{
    std::fstream configFile(configFilePath.string(),
                            std::fstream::out | std::fstream::trunc);
    if (configFile.is_open()) {
        ConfigurationParser::ptree defaultConfiguration;
        defaultConfiguration.put("core.repositoryformatversion", 0);
        defaultConfiguration.put("core.filemode", "false");
        defaultConfiguration.put("core.bare", "false");
        ConfigurationParser::write_ini(configFile, defaultConfiguration);
    }
    else {
        GENERATE_EXCEPTION("Coudn't open {}", configFilePath.string());
    }
}

std::optional<GitRepository>
GitRepository::findRootGitRepository(const GitRepository::Fpath& path,
                                     bool required)
{
    auto currentDir = Fs::canonical(path);

    if (auto gitDir = currentDir / ".git"; Fs::exists(gitDir)) {
        return GitRepository::create(path);
    }

    auto parentDir = currentDir.parent_path();
    if (parentDir == currentDir) {
        if (required) {
            GENERATE_EXCEPTION("Not a git directory {}", parentDir.string());
        }
        else {
            return {};
        }
    }
    return findRootGitRepository(parentDir, required);
}

GitRepository GitRepository::create(const Fpath& path, bool force)
{
    auto gitDir = path / ".git";
    if (!(force || Fs::exists(gitDir))) {
        GENERATE_EXCEPTION("Not a git repository {}", gitDir.string());
    }

    auto configPath = gitDir / "config";

    ConfigurationParser::ptree config;

    if (!configPath.empty() && Fs::exists(configPath)) {
        ConfigurationParser::ini_parser::read_ini(configPath, config);
    }
    else if (!force) {
        GENERATE_EXCEPTION("Configuration file {} is missing\n",
                           configPath.string());
    }

    if (!force) {
        // TODO: check int conversion
        auto version = config.get<int>("core.repositoryformatversion");
        if (version != 0) {
            GENERATE_EXCEPTION("Unsupported repositoryformatversion {}",
                               version);
        }
    }
    return GitRepository(path, gitDir, config);
}

GitRepository GitRepository::initialize(const Fpath& path)
{
    auto repository = create(path, true);

    if (Fs::exists(repository.m_workTree)) {
        if (!Fs::is_directory(repository.m_workTree)) {
            GENERATE_EXCEPTION("Not a directory: {}",
                               repository.m_workTree.string());
        }
        else if (!Fs::is_empty(repository.m_workTree)) {
            GENERATE_EXCEPTION("{} is not empty!",
                               Fs::absolute(repository.m_workTree).string());
        }
    }
    else {
        Fs::create_directories(repository.m_workTree);
    }

    assert(Fs::exists(repoDir(repository, CreateDir::YES, "branches")));
    assert(Fs::exists(repoDir(repository, CreateDir::YES, "objects")));
    assert(Fs::exists(repoDir(repository, CreateDir::YES, "refs", "tags")));
    assert(Fs::exists(repoDir(repository, CreateDir::YES, "refs", "heads")));

    try {
        std::string initialDescription =
            "Unnamed repository; edit this file 'description' to name the "
            "repository.\n";
        std::string headContent = "ref: refs/heads/master\n";

        writeToFile(repoPath(repository, "description"), initialDescription);
        writeToFile(repoPath(repository, "HEAD"), headContent);
        writeDefaultConfiguration(repoPath(repository, "config"));
    }
    catch (std::runtime_error ex) {
        throw ex;
    }

    return repository;
}

GitRepository::GitRepository(const Fpath& workTree, const Fpath& gitDir,
                             const ConfigurationParser::ptree& config)
    : m_workTree(workTree), m_gitDir(gitDir), m_configuration(config)
{
}

GitRepository::Fpath GitRepository::gitDir() const { return m_gitDir; }
}; // namespace Git