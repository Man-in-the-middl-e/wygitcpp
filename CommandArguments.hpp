#include "git_objects/GitIndex.hpp"
#include "git_objects/GitObject.hpp"
#include "git_objects/GitObjectsFactory.hpp"
#include "git_objects/GitRepository.hpp"

namespace GitCommands {
void init(const std::string& pathToGitRepository)
{
    auto repository = GitRepository::initialize(pathToGitRepository);
}

void catFile(const std::string& objectFormat,
             const std::string& objectReference)
{
    auto objectHash = Git::GitObject::findObject(objectReference, objectFormat);
    auto object = GitObjectFactory::read(objectHash);
    std::cout << object->serialize().data();
}

GitHash hashFile(const std::filesystem::path& path, const std::string& format,
                 bool write = true)
{
    auto fileContent = Utilities::readFile(path);
    auto gitObject = GitObjectFactory::create(format, fileContent);

    auto objectHash = Git::GitObject::write(gitObject.get(), write);
    return objectHash;
}

// TODO: figure out when commit can have multiple parents
void displayLog(const GitHash& hash)
{
    auto gitObject = GitObjectFactory::read(hash);
    assert(gitObject->format() == "commit");
    GitCommit* commit = static_cast<GitCommit*>(gitObject.get());

    auto& commitMessage = commit->commitMessage();

    auto authorEnds = commitMessage.author.find_last_of('>');
    if (authorEnds == std::string::npos) {
        GENERATE_EXCEPTION("Malformed date in {}, date should consist of time "
                           "since epoch followed by UTC offset",
                           commitMessage.author);
    }

    auto author = commitMessage.author.substr(0, authorEnds + 1);
    // auto date =
    // Utilities::decodeDateIn(commitMessage.author.substr(authorEnds + 2));
    std::cout << "commit: " << hash << std::endl;
    std::cout << "Author: " << author << std::endl;
    // std::cout << "Date:   " << date << std::endl;
    std::cout << "\n\t" << commitMessage.message << std::endl;

    if (commitMessage.parent.empty()) {
        return;
    }
    else {
        displayLog(GitHash(commitMessage.parent));
    }
}

void listTree(const GitHash& objectHash, const std::string& parentDir,
              bool recursive)
{
    auto gitObject = GitObjectFactory::read(objectHash);
    auto tree = static_cast<GitTree*>(gitObject.get());

    for (const auto& treeLeaf : tree->tree()) {
        try {
            auto childFormat = GitObjectFactory::read(treeLeaf.hash)->format();
            if (recursive && childFormat == "tree") {
                listTree(treeLeaf.hash, treeLeaf.filePath.filename(),
                         recursive);
            }
            else {
                std::cout << fmt::format(
                    "{0} {1} {2}\t{3}\n", treeLeaf.fileMode, childFormat,
                    treeLeaf.hash.data(),
                    (parentDir / treeLeaf.filePath).string());
            }
        }
        catch (std::runtime_error e) {
            std::cout << e.what() << std::endl;
        }
    }
}

void treeCheckout(const GitObject* object,
                  const std::filesystem::path& checkoutDirectory)
{
    auto treeObject = dynamic_cast<const GitTree*>(object);
    for (const auto& treeLeaf : treeObject->tree()) {
        auto childObject = GitObjectFactory::read(treeLeaf.hash);
        auto destination = checkoutDirectory / treeLeaf.filePath;

        if (childObject->format() == "tree") {
            std::filesystem::create_directories(destination);
            treeCheckout(childObject.get(), destination);
        }
        else if (childObject->format() == "blob") {
            Utilities::writeToFile(destination,
                                   childObject->serialize().data());
        }
    }
}

void cleanDirectory()
{
    std::vector<std::filesystem::path> entriesToRemove;
    for (const auto dirEntry : std::filesystem::directory_iterator(
             GitRepository::findRoot().workTree())) {
        auto dirEntryPath = dirEntry.path();
        if (dirEntry.is_directory() &&
            dirEntryPath.string().find("/.git") != std::string::npos) {
            continue;
        }
        entriesToRemove.push_back(dirEntryPath);
    }
    for (const auto& entry : entriesToRemove) {
        std::filesystem::remove_all(entry);
    }
}

void checkout(const std::string& branchOrCommit)
{
    cleanDirectory();
    auto commit = GitObject::findObject(branchOrCommit);
    auto gitObject =
        GitObjectFactory::read(GitObject::findObject(commit.data()));
    auto workTree = GitRepository::findRoot().workTree();

    // if is a branch
    if (auto pathToBranch = GitRepository::repoFile(
            GitRepository::findRoot(), "refs", "heads", branchOrCommit);
        std::filesystem::exists(pathToBranch)) {
        std::cout << fmt::format("Switched to branch: `{}`\n", branchOrCommit);
        GitRepository::setHEAD(branchOrCommit);
    }
    else {
        GitRepository::setHEAD(commit);
    }

    if (gitObject->format() == "commit") {
        auto gitCommit = static_cast<GitCommit*>(gitObject.get());
        auto treeHash = GitHash(gitCommit->commitMessage().tree);
        auto tree = GitObjectFactory::read(treeHash);
        treeCheckout(tree.get(), workTree);
    }
    else if (gitObject->format() == "tree") {
        treeCheckout(gitObject.get(), workTree);
    }
}

std::unordered_map<std::string, std::vector<std::filesystem::path>>
showReferences(const std::filesystem::path& refDir)
{
    std::unordered_map<std::string, std::vector<std::filesystem::path>> refs;
    for (auto const& dir_entry :
         std::filesystem::recursive_directory_iterator{refDir}) {
        if (dir_entry.is_regular_file()) {
            auto hash = GitObject::resolveReference(dir_entry.path());
            refs[hash].push_back(dir_entry.path());
        }
    }
    return refs;
}

void creatReference(const std::string& name, const GitHash& hash)
{
    auto repo = GitRepository::findRoot();
    auto referencePath = GitRepository::repoFile(repo, "refs", "tags", name);
    Utilities::writeToFile(referencePath, hash);
}

void createTag(const std::string& tagName, const GitHash& objectHash,
               bool createAssociativeTag)
{
    if (createAssociativeTag) {
        TagMessage tagMessage{
            .object = objectHash.data(),
            .type = "commit",
            .tag = tagName,
            .tagger = "Joe Doe <joedoe@email.com>",
            .gpgsig = "",
            .message =
                "A tag generated by wyag, which won't let you customize the "
                "message!"};
        auto tag = GitTag(tagMessage);
        auto tagSHA = GitObject::write(&tag);
        creatReference(tagName, tagSHA);
    }
    else {
        creatReference(tagName, objectHash);
    }
}

void listFiles()
{
    auto repo = GitRepository::findRoot();
    auto indexFile = GitRepository::repoFile(repo, "index");

    auto res = GitIndex::parse(indexFile);
    for (const auto& entry : res) {
        std::cout << entry.objectName << std::endl;
    }
}

GitHash createTree(const std::filesystem::path& dirPath)
{
    std::vector<GitTreeLeaf> leaves;
    for (auto dirEntry : std::filesystem::directory_iterator(dirPath)) {
        auto dirEntryPath = dirEntry.path();
        if (dirEntry.is_regular_file()) {
            leaves.push_back({.fileMode = GitTree::fileMode(dirEntry, "blob"),
                              .filePath = dirEntryPath.filename(),
                              .hash = hashFile(dirEntry.path(), "blob")});
        }
        else if (dirEntry.is_directory() && !dirPath.empty() &&
                 !dirEntryPath.string().ends_with(".git")) {
            // TODO: add support for the commit(submodules)
            leaves.push_back({.fileMode = GitTree::fileMode(dirEntry, "tree"),
                              .filePath = dirEntry,
                              .hash = createTree(dirEntryPath)});
        }
    }

    GitTree tree(leaves);
    auto treeHash = GitObject::write(&tree);
    return treeHash;
}

void commit(const std::string& message = "")
{
    auto rootRepo = GitRepository::findRoot();
    auto numberOfFiles =
        std::distance(std::filesystem::directory_iterator(rootRepo.workTree()),
                      std::filesystem::directory_iterator{});
    if (numberOfFiles == 1 && std::filesystem::exists(rootRepo.gitDir())) {
        std::cout << "There is nothing to commit" << std::endl;
        return;
    }

    auto getParent = [&]() {
        try {
            return GitRepository::HEAD();
        }
        catch (std::runtime_error error) {
            return std::string("");
        }
    };

    auto commitTree = createTree(rootRepo.workTree());
    CommitMessage commitMessage{.tree = commitTree.data(),
                                .parent = getParent(),
                                // TODO: add date to the author field
                                .author = "Joe Doe <joedoe@email.com>",
                                .committer = "joe Doe <joedoe@email.com>",
                                .gpgsig = "",
                                .message = message};

    GitCommit commitObject(commitMessage);
    auto commitHash = GitObject::write(&commitObject);
    GitRepository::commitToBranch(commitHash);

    if (auto head = GitRepository::HEAD();
        head.find("refs/") != std::string::npos) {
        std::cout << fmt::format("  [{} {}] committing\n",
                                 head.substr(head.find_last_of("/") + 1),
                                 commitHash.data().substr(0, 7));
    }
}

void createBranch(const std::string& branchName)
{
    auto currentCommit = GitRepository::HEAD();
    Utilities::writeToFile(GitRepository::repoPath(GitRepository::findRoot(),
                                                   "refs", "heads", branchName),
                           currentCommit);
}

void showBranches()
{
    auto root = GitRepository::findRoot();
    std::string currentBranch = GitRepository::currentBranch();
    if (currentBranch.empty()) {
        std::cout << fmt::format("* (HEAD detached at {})\n",
                                 GitRepository::HEAD().substr(0, 7));
    }

    auto pathToBranches = GitRepository::repoPath(root, "refs", "heads");
    for (const auto& dirEntry :
         std::filesystem::directory_iterator(pathToBranches)) {
        auto otherBranch = dirEntry.path().filename().string();
        std::cout << (currentBranch == otherBranch ? "* " : "  ")
                  << otherBranch << std::endl;
    }
}
}; // namespace GitCommands