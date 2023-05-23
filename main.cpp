#include "GitRepository.hpp"
#include "objects/GitObject.hpp"

#include <boost/program_options.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <iostream>
#include <sstream>

#include "utilities/SHA1.hpp"
#include "utilities/Zlib.hpp"

void catFile(const std::string& objectFormat, const std::string& sha)
{
    auto repository = Git::GitRepository::findRootGitRepository("gitTest");

    if (repository.has_value()) {
        auto objectLocation = Git::GitObject::findObject(
            repository.value(), objectFormat, sha);
        auto object =
            Git::GitObject::read(repository.value(), objectLocation.string());
        std::cout << object->serialize().data();
    }
}

namespace po = boost::program_options;
int main(int argc, char* argv[])
{
    po::options_description desc("Allowed options");
    // TODO: add more robust checks and meaningful description
    // clang-format off
    desc.add_options()
        ("help",
            "Produce help message")
        ("init",
            po::value<std::string>(),
            "Create an empty Git repository")
        ("cat-file",
            po::value<std::vector<std::string>>()->multitoken()->composing(),
            "Provide content of repository objects");
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
                Git::GitRepository::initialize(pathToGitRepository);
        }
        else if (vm.count("cat-file")) {
            auto catFileArguments =
                vm["cat-file"].as<std::vector<std::string>>();
            if (catFileArguments.size() == 2) {
                catFile(catFileArguments[0], catFileArguments[1]);
            }
        }
    }
    catch (std::runtime_error myex) {
        std::cout << myex.what() << std::endl;
    }
    return 0;
}