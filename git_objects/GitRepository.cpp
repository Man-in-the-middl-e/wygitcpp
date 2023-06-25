
#include "GitRepository.hpp"

#include <assert.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "GitObject.hpp"
#include <iostream>

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
        GENERATE_EXCEPTION("Couldn't open {}", configFilePath.string());
    }
}

GitRepository GitRepository::findRoot(const GitRepository::Fpath& path)
{
    auto currentDir = Fs::canonical(path);
    if (auto gitDir = currentDir / ".git"; Fs::exists(gitDir)) {
        static GitRepository repo = GitRepository::create(path);
        return repo;
    }

    auto parentDir = currentDir.parent_path();
    if (parentDir == currentDir) {
        GENERATE_EXCEPTION("Couldn't find .git directory {}",
                           Fs::absolute(parentDir).string());
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
            "repository.";
        std::string headContent = "ref: refs/heads/master";

        Utilities::writeToFile(repoPath(repository, "description"),
                               initialDescription, true);
        Utilities::writeToFile(repoPath(repository, "HEAD"), headContent, true);
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

GitRepository::Fpath GitRepository::pathToHead()
{
    static Fpath pathToHead = repoPath(findRoot(), "HEAD");
    return pathToHead;
}
void GitRepository::commitToBranch(const GitHash& commitHash)
{
    auto currentHead = HEAD(HeadType::REF);
    if (currentHead.starts_with("ref: ")) {
        auto pathToBranch = currentHead.substr(currentHead.find(' ') + 1);
        Utilities::writeToFile(repoPath(findRoot(), pathToBranch), commitHash);
    }
    else {
        std::cout << "This commit doesn't belong to any branch\n";
        setHEAD(commitHash);
    }
}

void GitRepository::setHEAD(const std::string& value)
{

    Utilities::writeToFile(GitRepository::pathToHead(),
                           fmt::format("ref: refs/heads/{}", value), true);
}

void GitRepository::setHEAD(const GitHash& hash)
{

    Utilities::writeToFile(GitRepository::pathToHead(), hash);
}

std::string GitRepository::HEAD(HeadType headType)
{
    return GitObject::resolveReference(pathToHead(),
                                       headType == HeadType::HASH);
}

std::string GitRepository::currentBranch()
{
    if (auto head = HEAD(HeadType::REF); head.starts_with("ref: ")) {
        return head.substr(head.find_last_of('/') + 1);
    }
    return "";
}

const GitRepository::Fpath& GitRepository::gitDir() const { return m_gitDir; }
const GitRepository::Fpath& GitRepository::workTree() const
{
    return m_workTree;
}
}; // namespace Git