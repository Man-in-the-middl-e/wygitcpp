
#include <boost/program_options.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <iostream>
#include <sstream>

#include "git_objects/GitObject.hpp"
#include "git_objects/GitRepository.hpp"
#include "utilities/SHA1.hpp"
#include "utilities/Zlib.hpp"

void catFile(const std::string& objectFormat, const GitHash& objectHash)
{
    auto repository = GitRepository::findRootGitRepository(".");

    if (repository.has_value()) {
        auto objectLocation = Git::GitObject::findObject(
            repository.value(), objectHash.data(), objectFormat);
        auto object =
            Git::GitObject::read(repository.value(), GitHash(objectLocation.string()));
        std::cout << object->serialize().data();
    }
}

void hashFile(const std::string& path, const std::string& format, bool write)
{
    auto fileContent = Utilities::readFile(path);
    auto repository = GitRepository::create(".");
    auto gitObject = Git::GitObject::create(format, repository, fileContent);

    auto objectHash = Git::GitObject::write(gitObject.get(), write);
    std::cout << "Object ID was created: " << objectHash << std::endl;
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
            auto repository =
                GitRepository::initialize(pathToGitRepository);
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
    }
    catch (std::runtime_error myex) {
        std::cout << myex.what() << std::endl;
    }
    return 0;
}