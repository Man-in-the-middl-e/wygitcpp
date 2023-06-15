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
    std::cout << "commit: " << hash.data().data() << std::endl;
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

void checkout(const GitHash& commit,
              const std::filesystem::path& checkoutDirectory)
{
    auto gitObject =
        GitObjectFactory::read(GitObject::findObject(commit.data()));

    if (std::filesystem::exists(checkoutDirectory)) {
        if (!std::filesystem::is_directory(checkoutDirectory)) {
            GENERATE_EXCEPTION("{}, is not a directory!",
                               checkoutDirectory.string());
        }
        else if (!std::filesystem::is_empty(checkoutDirectory)) {
            GENERATE_EXCEPTION(
                "{}, is not an empty directory. Currently checkout only works "
                "with the empty directories",
                checkoutDirectory.string());
        }
    }
    else {
        std::filesystem::create_directories(checkoutDirectory);
    }

    if (gitObject->format() == "commit") {
        auto gitCommit = static_cast<GitCommit*>(gitObject.get());
        auto treeHash = GitHash(gitCommit->commitMessage().tree);
        auto tree = GitObjectFactory::read(treeHash);
        treeCheckout(tree.get(), checkoutDirectory);
    }
    else if (gitObject->format() == "tree") {
        treeCheckout(gitObject.get(), checkoutDirectory);
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
    Utilities::writeToFile(referencePath, hash.data() + "\n");
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
    if (numberOfFiles == 1 &&
        std::filesystem::exists(rootRepo.gitDir())) {
        std::cout << "There is nothing to commit" << std::endl;
        return;
    }

    auto getParent = [&]() {
        try {
            auto currentHead = GitObject::resolveReference(
                GitRepository::repoPath(rootRepo, "HEAD"));
            return currentHead;
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
    auto currentBranchHead = GitRepository::repoPath(
        rootRepo, "refs", "heads", GitRepository::currentBranch());
    Utilities::writeToFile(currentBranchHead, commitHash.data());
    std::cout << fmt::format("  [{} {}] committing\n",
                             GitRepository::currentBranch(),
                             commitHash.data().substr(0, 7));
}
}; // namespace GitCommands