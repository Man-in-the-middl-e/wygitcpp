
#include "GitRepository.hpp"

#include <assert.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace Git {
namespace ConfigurationParser = boost::property_tree;

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

GitRepository
GitRepository::findRoot(const GitRepository::Fpath& path)
{
    auto currentDir = Fs::canonical(path);
    if (auto gitDir = currentDir / ".git"; Fs::exists(gitDir)) {
        static GitRepository repo = GitRepository::create(path);
        return repo;
    }

    auto parentDir = currentDir.parent_path();
    if (parentDir == currentDir) {
        GENERATE_EXCEPTION("Coulnd't find git direcotry {}", parentDir.string());
    }
    return findRoot(parentDir);
}

GitRepository GitRepository::create(const Fpath& path)
{
    return GitRepository(path, path / ".git");
}

GitRepository GitRepository::initialize(const Fpath& path)
{
    auto repository = create(path);

    if (Fs::exists(repository.m_workTree)) {
        if (!Fs::is_directory(repository.m_workTree)) {
            GENERATE_EXCEPTION("Not a directory: {}",
                               repository.m_workTree.string());
        }
        else if (!Fs::is_empty(repository.m_workTree)) {
            GENERATE_EXCEPTION("{} is already a git repository!",
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

        Utilities::writeToFile(repoPath(repository, "description"),
                               initialDescription);
        Utilities::writeToFile(repoPath(repository, "HEAD"), headContent);
        writeDefaultConfiguration(repoPath(repository, "config"));
    }
    catch (std::runtime_error ex) {
        throw ex;
    }

    return repository;
}

GitRepository::GitRepository(const Fpath& workTree, const Fpath& gitDir)
    : m_workTree(workTree), m_gitDir(gitDir)
{
}

GitRepository::Fpath GitRepository::gitDir() const { return m_gitDir; }
}; // namespace Git