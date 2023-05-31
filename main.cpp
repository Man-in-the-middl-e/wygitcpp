
#include <boost/program_options.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <iostream>
#include <sstream>

#include "git_objects/GitObject.hpp"
#include "git_objects/GitObjectsFactory.hpp"
#include "git_objects/GitRepository.hpp"

#include "utilities/SHA1.hpp"
#include "utilities/Zlib.hpp"
#include <unordered_map>

void catFile(const std::string& objectFormat, const GitHash& objectHash)
{
    auto repository = GitRepository::findRootGitRepository(".");

    if (repository.has_value()) {
        auto objectLocation = Git::GitObject::findObject(
            repository.value(), objectHash.data(), objectFormat);
        auto object = GitObjectFactory::read(repository.value(),
                                             GitHash(objectLocation.string()));
        std::cout << object->serialize().data();
    }
}

void hashFile(const std::string& path, const std::string& format, bool write)
{
    auto fileContent = Utilities::readFile(path);
    auto repository = GitRepository::create(".");
    auto gitObject = GitObjectFactory::create(format, repository, fileContent);

    auto objectHash = Git::GitObject::write(gitObject.get(), write);
    std::cout << "Object ID was created: " << objectHash << std::endl;
}

// TODO: figure out when commit can have multiple parents
void dispalyLog(const GitRepository& repo, const GitHash& hash)
{
    auto gitObject = GitObjectFactory::read(repo, hash);
    GitCommit* commit = dynamic_cast<GitCommit*>(gitObject.get());
    assert(commit != nullptr);

    auto& commitMessage = commit->message();

    auto authorEnds = commitMessage.author.find_last_of('>');
    if (authorEnds == std::string::npos) {
        GENERATE_EXCEPTION("Malformed date in {}, date should consist of time "
                           "since epoch followed by UTC offset",
                           commitMessage.author);
    }

    auto author = commitMessage.author.substr(0, authorEnds + 1);
    auto date =
        Utilities::decodeDateIn(commitMessage.author.substr(authorEnds + 2));
    std::cout << "commit: " << hash.data().data() << std::endl;
    std::cout << "Author: " << author << std::endl;
    std::cout << "Date:   " << date << std::endl;
    std::cout << "\n\t" << commitMessage.messaage << std::endl;

    if (commitMessage.parent.empty()) {
        return;
    }
    else {
        dispalyLog(repo, GitHash(commitMessage.parent));
    }
}

void listTree(const std::string& objectHash)
{
    auto repo = GitRepository::findRootGitRepository().value();

    auto object = GitObject::findObject(repo, objectHash, "tree");
    auto gitObject = GitObjectFactory::read(repo, GitHash(object));
    auto tree = dynamic_cast<GitTree*>(gitObject.get());
    assert(tree != nullptr);

    for (const auto& treeLeaf : tree->tree()) {
        auto childFormat = GitObjectFactory::read(repo, treeLeaf.hash);
        std::cout << fmt::format("{0} {1} {2}\t{3}\n", treeLeaf.fileMode,
                                 childFormat->format(), treeLeaf.hash.data(),
                                 treeLeaf.filePath.string());
    }
}

void treeCheckout(const GitObject* object, const GitRepository& repo,
                  const std::filesystem::path& checkoutDirectory)
{
    auto treeObject = dynamic_cast<const GitTree*>(object);
    for (const auto& treeLeaf : treeObject->tree()) {
        auto childObject = GitObjectFactory::read(repo, treeLeaf.hash);
        auto destination = checkoutDirectory / treeLeaf.filePath;

        if (childObject->format() == "tree") {
            std::filesystem::create_directories(destination);
            treeCheckout(childObject.get(), repo, destination);
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
    auto repo = GitRepository::findRootGitRepository().value();
    auto gitObject = GitObjectFactory::read(
        repo, GitHash(GitObject::findObject(repo, commit.data())));

    if (std::filesystem::exists(checkoutDirectory)) {
        if (!std::filesystem::is_directory(checkoutDirectory)) {
            GENERATE_EXCEPTION("{}, is not a directory!",
                               checkoutDirectory.string());
        }
        else if (!std::filesystem::is_empty(checkoutDirectory)) {
            GENERATE_EXCEPTION(
                "{}, is not an empyt directory. Currently checkout only works "
                "with the empty directories",
                checkoutDirectory.string());
        }
    }
    else {
        std::filesystem::create_directories(checkoutDirectory);
    }

    // TODO: avoid dynamic_cast by impelemnting read<T> function
    if (auto gitCommit = dynamic_cast<GitCommit*>(gitObject.get()); gitCommit) {
        auto treeHash = GitHash(gitCommit->message().tree);
        auto tree = GitObjectFactory::read(repo, treeHash);
        treeCheckout(tree.get(), repo, checkoutDirectory);
    }
    else {
        treeCheckout(gitObject.get(), repo, checkoutDirectory);
    }
}

std::string resolveReference(const std::filesystem::path& reference)
{
    auto referenceContet = Utilities::readFile(reference);
    if (referenceContet.starts_with("ref: ")) {
        return resolveReference(referenceContet.substr(5));
    }
    else {
        referenceContet.erase(referenceContet.end() - 1);
        return referenceContet;
    }
}

std::unordered_map<std::string, std::vector<std::string>>
showReferences(const std::filesystem::path& refDir)
{
    std::unordered_map<std::string, std::vector<std::string>> refs;
    for (auto const& dir_entry :
         std::filesystem::recursive_directory_iterator{refDir}) {
        if (dir_entry.is_regular_file()) {
            auto hash = resolveReference(dir_entry.path());
            refs[hash].push_back(dir_entry.path().string());
        }
    }
    return refs;
}

namespace po = boost::program_options;
int main(int argc, char* argv[])
{
    po::options_description desc("Allowed options");
    // TODO: add more robust checks and meaningful description
    // clang-format off
    desc.add_options()
        ("help",
            "Produce help message"
        )
        ("init",
            po::value<std::string>(),
            "Create an empty Git repository"
        )
        ("cat-file",
            po::value<std::vector<std::string>>()->multitoken()->composing(),
            "Provide content of repository objects"
        )
        ("hash-file",
            po::value<std::vector<std::string>>()->multitoken()->composing(),
            "Compute object ID and optionally creates a blob from a file"
        )
        ("log",
            po::value<std::string>(),
            "Display history of a given commit."
        )
        ("ls-tree",
            po::value<std::string>(),
            "Pretty-print a tree object."
        )
        ("checkout",
            po::value<std::vector<std::string>>()->multitoken()->composing(),
            "Checkout a commit inside of a directory."
        )
        ("show-ref",
            po::value<std::string>()->implicit_value(".git/refs"),
            "List references."
        );
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || argc < 2) {
        std::cout << desc << "\n";
        return 1;
    }

    try {
        if (vm.count("init")) {
            std::string pathToGitRepository = vm["init"].as<std::string>();
            auto repository = GitRepository::initialize(pathToGitRepository);
        }
        else if (vm.count("cat-file")) {
            auto catFileArguments =
                vm["cat-file"].as<std::vector<std::string>>();
            if (catFileArguments.size() == 2) {
                catFile(catFileArguments[0], GitHash(catFileArguments[1]));
            }
        }
        else if (vm.count("hash-file")) {
            auto hashFileArguments =
                vm["hash-file"].as<std::vector<std::string>>();
            if (hashFileArguments.size() == 2) {
                // TODO: make wirte opitonal argument
                hashFile(hashFileArguments[0], hashFileArguments[1], true);
            }
        }
        else if (vm.count("log")) {
            auto commitHash = vm["log"].as<std::string>();
            auto repo = GitRepository::findRootGitRepository();
            auto object = GitObject::findObject(repo.value(), commitHash);
            dispalyLog(repo.value(), GitHash(object));
        }
        else if (vm.count("ls-tree")) {
            auto objectHash = vm["ls-tree"].as<std::string>();
            listTree(objectHash);
        }
        else if (vm.count("checkout")) {
            auto checkoutArguments =
                vm["checkout"].as<std::vector<std::string>>();
            if (checkoutArguments.size() == 2) {
                auto hash = GitHash(checkoutArguments[0]);
                auto checkoutDir =
                    std::filesystem::absolute(checkoutArguments[1]);
                checkout(hash, checkoutDir);
            }
        }
        else if (vm.count("show-ref")) {
            auto path = vm["show-ref"].as<std::string>();
            for (const auto& [hash, refs] : showReferences(path)) {
                for (const auto& ref : refs) {
                    std::cout << hash << '\t' << ref << std::endl;
                }
            }
        }
    }
    catch (std::runtime_error myex) {
        std::cout << myex.what() << std::endl;
    }

    return 0;
}