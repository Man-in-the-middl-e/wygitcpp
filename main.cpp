#include <boost/program_options.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <iostream>

#include "CommandArguments.hpp"
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
            po::value<std::string>()->implicit_value("."),
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
            po::value<std::string>()->implicit_value("HEAD"),
            "Display history of a given commit."
        )
        ("ls-tree",
            po::value<std::vector<std::string>>()->multitoken()->composing(),
            "Pretty-print a tree object."
        )
        ("checkout",
            po::value<std::vector<std::string>>()->multitoken()->composing(),
            "Checkout a commit inside of a directory."
        )
        ("show-ref",
            po::value<std::string>()->implicit_value(".git/refs"),
            "List references."
        )
        // TODO: there is no ls-tag, make it part of tag somehow
        ("ls-tag",
            po::value<std::string>()->implicit_value(""),
            "List tags"
        )
        ("tag",
            po::value<std::vector<std::string>>()->multitoken()->composing(),
            "Create tag, use -a to create a tag object"
        )
        ("rev-parse",
            po::value<std::vector<std::string>>()->multitoken()->composing(),
            "Parse revision (or other objects )identifiers"
        )
        ("ls-files",
            po::value<std::string>()->implicit_value(""),
            "List all the stage files"
        )
        ("commit",
            po::value<std::string>()->implicit_value("."),
            "Create commit"
        )
        ;
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
            GitCommands::init(pathToGitRepository);
        }
        else if (vm.count("cat-file")) {
            auto catFileArguments =
                vm["cat-file"].as<std::vector<std::string>>();
            if (catFileArguments.size() == 2) {
                GitCommands::catFile(catFileArguments[0], catFileArguments[1]);
            }
        }
        else if (vm.count("hash-file")) {
            auto hashFileArguments =
                vm["hash-file"].as<std::vector<std::string>>();
            if (hashFileArguments.size() == 2) {
                // TODO: make write optional argument
                GitCommands::hashFile(hashFileArguments[0],
                                      hashFileArguments[1], true);
            }
        }
        else if (vm.count("log")) {
            auto commitHash = vm["log"].as<std::string>();
            auto object = GitObject::findObject(commitHash);
            GitCommands::displayLog(object);
        }
        else if (vm.count("ls-tree")) {
            auto lsTreeArguments = vm["ls-tree"].as<std::vector<std::string>>();
            auto objectHash = GitObject::findObject(lsTreeArguments[0], "tree");
            auto recursive =
                lsTreeArguments.size() > 1 && lsTreeArguments[1] == "r";
            GitCommands::listTree(objectHash, "", recursive);
        }
        else if (vm.count("checkout")) {
            auto checkoutArguments =
                vm["checkout"].as<std::vector<std::string>>();
            if (checkoutArguments.size() == 1) {
                auto hash = GitObject::findObject(checkoutArguments[0]);
                GitCommands::checkout(hash);
            }
        }
        else if (vm.count("show-ref")) {
            auto path = vm["show-ref"].as<std::string>();
            for (const auto& [hash, refs] : GitCommands::showReferences(path)) {
                for (const auto& ref : refs) {
                    std::cout << hash << '\t' << ref.string() << std::endl;
                }
            }
        }
        else if (vm.count("ls-tag")) {
            auto tagsPath = GitRepository::repoFile(GitRepository::findRoot(),
                                                    "refs", "tags");
            if (!tagsPath.empty()) {
                for (const auto& [_, tags] :
                     GitCommands::showReferences(tagsPath)) {
                    for (const auto& tag : tags) {
                        std::cout << tag.filename().string() << std::endl;
                    }
                }
            }
        }
        else if (vm.count("tag")) {
            auto tagArguments = vm["tag"].as<std::vector<std::string>>();
            if (tagArguments.size() >= 2) {
                auto tagName = tagArguments[0];
                auto objectHash = GitObject::findObject(tagArguments[1]);
                bool createAssociativeTag =
                    tagArguments.size() == 3 && tagArguments[2] == "a";
                GitCommands::createTag(tagName, objectHash,
                                       createAssociativeTag);
            }
        }
        else if (vm.count("rev-parse")) {
            auto revParseArgs = vm["rev-parse"].as<std::vector<std::string>>();
            if (revParseArgs.size() >= 1) {
                auto objectName = revParseArgs[0];
                auto fmt = revParseArgs.size() > 1 ? revParseArgs[1] : "";
                std::cout << GitObject::findObject(objectName, fmt)
                          << std::endl;
            }
        }
        else if (vm.count("ls-files")) {
            GitCommands::listFiles();
        }
        else if (vm.count("commit")) {
            GitCommands::commit("Auto generated commit message");
        }
    }
    catch (std::runtime_error myex) {
        std::cout << myex.what() << std::endl;
    }
};
