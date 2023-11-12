#include <argparse/argparse.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <iostream>

#include "GitCommands.hpp"

// TODO: remove when this bug is fixed and use .choices() instead
// https://github.com/p-ranav/argparse/issues/307
std::string verifyType(const std::string& type)
{
    if (type != "commit" && type != "blob" && type != "tree" && type != "tag") {
        GENERATE_EXCEPTION("Wrong type: {}, available types: "
                           "[commit, blob, tree, tag]",
                           type);
    }
    return type;
}

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("wygit");

    // clang-format off
    argparse::ArgumentParser initCommand("init");
    initCommand.add_description("Initialize a new, empty git repository.");
    initCommand.add_argument("path")
               .help("Path to directory where you want to create repository.")
               .metavar("directory")
               .nargs(argparse::nargs_pattern::optional)
               .default_value(".");
    
    argparse::ArgumentParser catFileCommand("cat-file");
    catFileCommand.add_description("Provide content of repository objects");
    catFileCommand.add_argument("type")
                  .help("Specify the type")
                  .metavar("type");
    catFileCommand.add_argument("object")
                  .help("The object to display")
                  .metavar("object");

    argparse::ArgumentParser hashObjectCommand("hash-object");
    hashObjectCommand.add_description("Compute object ID and optionally creates a blob from a file");
    hashObjectCommand.add_argument("path")
                     .help("Read object from file")
                     .metavar("path");
    hashObjectCommand.add_argument("-t")
                     .help("Specify the type")
                     .metavar("type")
                     .default_value("blob");
    hashObjectCommand.add_argument("-w")
                     .help("Actually write the object into the database")
                     .flag();
    argparse::ArgumentParser logCommand("log");
    logCommand.add_description("Display history of a given commit");
    logCommand.add_argument("commit")
               .help("Commit to start at")
               .metavar("commit")
               .default_value("HEAD");

    program.add_subparser(initCommand);
    program.add_subparser(catFileCommand);
    program.add_subparser(hashObjectCommand);
    program.add_subparser(logCommand);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& myEx) {
        std::cerr << myEx.what() << std::endl << program;
        std::exit(EXIT_FAILURE);
    }
    // clang-format on

    try {
        if (program.is_subcommand_used("init")) {
            std::string pathToRepository =
                program.at<argparse::ArgumentParser>("init").get<std::string>(
                    "path");
            GitCommands::init(pathToRepository);
        }
        else if (program.is_subcommand_used("cat-file")) {
            auto& catFileSubParser =
                program.at<argparse::ArgumentParser>("cat-file");
            auto objectFormat =
                verifyType(catFileSubParser.get<std::string>("type"));

            auto objectToDisplay = catFileSubParser.get<std::string>("object");
            GitCommands::catFile(objectFormat, objectToDisplay);
        }
        else if (program.is_subcommand_used("hash-object")) {
            auto& hashObjectSubParser =
                program.at<argparse::ArgumentParser>("hash-object");
            auto pathToFile = hashObjectSubParser.get<std::string>("path");
            auto objectType =
                verifyType(hashObjectSubParser.get<std::string>("-t"));
            auto writeToFile = hashObjectSubParser.get<bool>("-w");
            std::cout << GitCommands::hashObject(pathToFile, objectType,
                                                 writeToFile)
                      << std::endl;
        }
        else if (program.is_subcommand_used("log")) {
            auto commit =
                program.at<argparse::ArgumentParser>("log").get<std::string>(
                    "commit");
            auto object = GitObject::findObject(commit);
            GitCommands::displayLog(object);
        }
        else {
            std::cout << program.help().str() << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    catch (const std::exception& myEx) {
        std::cerr << myEx.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;

    // po::options_description desc("Allowed options");
    // // TODO: add more robust checks and meaningful description
    // // clang-format off
    // desc.add_options()
    //     ("help",
    //         "Produce help message"
    //     )
    //     ("init",
    //         po::value<std::string>()->implicit_value("."),
    //         "Create an empty Git repository"
    //     )
    //     ("cat-file",
    //         po::value<std::vector<std::string>>()->multitoken()->composing(),
    //         "Provide content of repository objects"
    //     )
    //     ("hash-file",
    //         po::value<std::vector<std::string>>()->multitoken()->composing(),
    //         "Compute object ID and optionally creates a blob from a file"
    //     )
    //     ("log",
    //         po::value<std::string>()->implicit_value("HEAD"),
    //         "Display history of a given commit."
    //     )
    //     ("ls-tree",
    //         po::value<std::vector<std::string>>()->multitoken()->composing(),
    //         "Pretty-print a tree object."
    //     )
    //     ("checkout",
    //         po::value<std::vector<std::string>>()->multitoken()->composing(),
    //         "Checkout a commit inside of a directory."
    //     )
    //     ("show-ref",
    //         po::value<std::string>()->implicit_value(".git/refs"),
    //         "List references."
    //     )
    //     // TODO: there is no ls-tag, make it part of tag somehow
    //     ("ls-tag",
    //         po::value<std::string>()->implicit_value(""),
    //         "List tags"
    //     )
    //     ("tag",
    //         po::value<std::vector<std::string>>()->multitoken()->composing(),
    //         "Create tag, use -a to create a tag object"
    //     )
    //     ("rev-parse",
    //         po::value<std::vector<std::string>>()->multitoken()->composing(),
    //         "Parse revision (or other objects )identifiers"
    //     )
    //     ("ls-files",
    //         po::value<std::string>()->implicit_value(""),
    //         "List all the stage files"
    //     )
    //     ("commit",
    //         po::value<std::string>()->implicit_value("."),
    //         "Create commit"
    //     )
    //     ("branch",
    //         po::value<std::string>()->implicit_value(""),
    //         "Create branch"
    //     );
    //     ;
    // // clang-format on

    // po::variables_map vm;
    // po::store(po::parse_command_line(argc, argv, desc), vm);
    // po::notify(vm);

    // if (vm.count("help") || argc < 2) {
    //     std::cout << desc << "\n";
    //     return 1;
    // }

    // try {
    //     if (vm.count("init")) {
    //         std::string pathToGitRepository = vm["init"].as<std::string>();
    //         GitCommands::init(pathToGitRepository);
    //     }
    //     else if (vm.count("cat-file")) {
    //         auto catFileArguments =
    //             vm["cat-file"].as<std::vector<std::string>>();
    //         if (catFileArguments.size() == 2) {
    //             GitCommands::catFile(catFileArguments[0],
    //             catFileArguments[1]);
    //         }
    //     }
    //     else if (vm.count("hash-file")) {
    //         auto hashFileArguments =
    //             vm["hash-file"].as<std::vector<std::string>>();
    //         if (hashFileArguments.size() == 2) {
    //             // TODO: make write optional argument
    //             GitCommands::hashFile(hashFileArguments[0],
    //                                   hashFileArguments[1], true);
    //         }
    //     }
    //     else if (vm.count("log")) {
    //         auto commitHash = vm["log"].as<std::string>();
    //         auto object = GitObject::findObject(commitHash);
    //         GitCommands::displayLog(object);
    //     }
    //     else if (vm.count("ls-tree")) {
    //         auto lsTreeArguments =
    //         vm["ls-tree"].as<std::vector<std::string>>(); auto objectHash =
    //         GitObject::findObject(lsTreeArguments[0], "tree"); auto recursive
    //         =
    //             lsTreeArguments.size() > 1 && lsTreeArguments[1] == "r";
    //         GitCommands::listTree(objectHash, "", recursive);
    //     }
    //     else if (vm.count("checkout")) {
    //         auto checkoutArguments =
    //             vm["checkout"].as<std::vector<std::string>>();
    //         if (checkoutArguments.size() == 1) {
    //             GitCommands::checkout(checkoutArguments[0]);
    //         }
    //     }
    //     else if (vm.count("show-ref")) {
    //         auto path = vm["show-ref"].as<std::string>();
    //         for (const auto& [hash, refs] :
    //         GitCommands::showReferences(path)) {
    //             for (const auto& ref : refs) {
    //                 std::cout << hash << '\t' << ref.string() << std::endl;
    //             }
    //         }
    //     }
    //     else if (vm.count("ls-tag")) {
    //         auto tagsPath = GitRepository::repoFile("refs", "tags");
    //         if (!tagsPath.empty()) {
    //             for (const auto& [_, tags] :
    //                  GitCommands::showReferences(tagsPath)) {
    //                 for (const auto& tag : tags) {
    //                     std::cout << tag.filename().string() << std::endl;
    //                 }
    //             }
    //         }
    //     }
    //     else if (vm.count("tag")) {
    //         auto tagArguments = vm["tag"].as<std::vector<std::string>>();
    //         if (tagArguments.size() >= 2) {
    //             auto tagName = tagArguments[0];
    //             auto objectHash = GitObject::findObject(tagArguments[1]);
    //             bool createAssociativeTag =
    //                 tagArguments.size() == 3 && tagArguments[2] == "a";
    //             GitCommands::createTag(tagName, objectHash,
    //                                    createAssociativeTag);
    //         }
    //     }
    //     else if (vm.count("rev-parse")) {
    //         auto revParseArgs =
    //         vm["rev-parse"].as<std::vector<std::string>>(); if
    //         (revParseArgs.size() >= 1) {
    //             auto objectName = revParseArgs[0];
    //             auto fmt = revParseArgs.size() > 1 ? revParseArgs[1] : "";
    //             std::cout << GitObject::findObject(objectName, fmt)
    //                       << std::endl;
    //         }
    //     }
    //     else if (vm.count("ls-files")) {
    //         GitCommands::listFiles();
    //     }
    //     else if (vm.count("commit")) {
    //         GitCommands::commit("Auto generated commit message");
    //     }
    //     else if (vm.count("branch")) {
    //         auto branchName = vm["branch"].as<std::string>();
    //         if (branchName.empty()) {
    //             GitCommands::showBranches();
    //         }
    //         else {
    //             GitCommands::createBranch(branchName);
    //         }
    //     }
    // }
    // catch (std::runtime_error myex) {
    //     std::cout << myex.what() << std::endl;
    // }
}
