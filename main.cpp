#include "GitRepository.hpp"
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;
int main(int argc, char* argv[])
{
    std::string pathToGitRepository;
    po::options_description desc("Allowed options");
    desc.add_options()("help", "Produce help message")(
        "init",
        po::value<std::string>(&pathToGitRepository)->default_value("."),
        "Create an empty Git repository");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || argc < 2) {
        std::cout << desc << "\n";
        return 1;
    }

    try {
        if (vm.count("init")) {
            auto repository =
                Git::GitRepository::initialize(pathToGitRepository);
        }
    }
    catch (std::runtime_error myex) {
        std::cout << myex.what() << std::endl;
    }

    return 0;
}