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
    catFileCommand.add_description("Provide content of repository objects.");
    catFileCommand.add_argument("type")
                  .help("Specify the type.")
                  .metavar("type");
    catFileCommand.add_argument("object")
                  .help("The object to display.")
                  .metavar("object");

    argparse::ArgumentParser hashObjectCommand("hash-object");
    hashObjectCommand.add_description("Compute object ID and optionally creates a blob from a file");
    hashObjectCommand.add_argument("path")
                     .help("Read object from file.")
                     .metavar("path");
    hashObjectCommand.add_argument("-t")
                     .help("Specify the type.")
                     .metavar("type")
                     .default_value("blob");
    hashObjectCommand.add_argument("-w")
                     .help("Actually write the object into the database.")
                     .flag();
    
    argparse::ArgumentParser logCommand("log");
    logCommand.add_description("Display history of a given commit.");
    logCommand.add_argument("commit")
               .help("Commit to start at.")
               .metavar("commit")
               .default_value("HEAD");

    argparse::ArgumentParser lsTreeCommand("ls-tree");
    lsTreeCommand.add_description("Pretty-print a tree object.");
    lsTreeCommand.add_argument("tree")
                 .help("A tree-ish object.")
                 .metavar("tree");
    lsTreeCommand.add_argument("-r")
                 .help("Recurse into sub-trees.")
                 .flag();
    
    argparse::ArgumentParser showRefCommand("show-ref");
    showRefCommand.add_description("List references.");

    argparse::ArgumentParser tagCommand("tag");
    tagCommand.add_description("List and create tags.");
    tagCommand.add_argument("name")
              .help("Name of the tag.")
              .nargs(argparse::nargs_pattern::optional);
    tagCommand.add_argument("object")
              .help("The object the new tag will point to.")
              .default_value("HEAD");
    tagCommand.add_argument("-a")
              .help("Whether to create a tag object")
              .flag();

    argparse::ArgumentParser revParseCommand("rev-parse");
    revParseCommand.add_description("Parse revision (or other objects) identifiers.");
    revParseCommand.add_argument("name")
                   .help("The name to parse.");

    argparse::ArgumentParser lsFilesCommand("ls-files");
    lsFilesCommand.add_description("List all the stage files.");

    argparse::ArgumentParser commitCommand("commit");
    commitCommand.add_description("Record changes to the repository.");
    commitCommand.add_argument("-m")
                 .help("Message to associate with this commit.")
                 .metavar("message");
    
    argparse::ArgumentParser branchCommand("branch");
    branchCommand.add_description("List, create branches.");
    branchCommand.add_argument("name")
                 .help("Provide the name of a branch.")
                 .nargs(argparse::nargs_pattern::optional);

    argparse::ArgumentParser checkoutCommand("checkout");
    checkoutCommand.add_description("Checkout a commit inside of a directory.");
    checkoutCommand.add_argument("commit")
                   .help("Commit to checkout to.");

    program.add_subparser(initCommand);
    program.add_subparser(catFileCommand);
    program.add_subparser(hashObjectCommand);
    program.add_subparser(logCommand);
    program.add_subparser(lsTreeCommand);
    program.add_subparser(showRefCommand);
    program.add_subparser(tagCommand);
    program.add_subparser(revParseCommand);
    program.add_subparser(lsFilesCommand);
    program.add_subparser(commitCommand);
    program.add_subparser(branchCommand);
    program.add_subparser(checkoutCommand);

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
        else if (program.is_subcommand_used("ls-tree")) {
            auto& lsTreeSubParser =
                program.at<argparse::ArgumentParser>("ls-tree");
            auto objectHash = GitObject::findObject(
                lsTreeSubParser.get<std::string>("tree"), "tree");
            auto recursive = lsTreeSubParser.get<bool>("-r");
            GitCommands::listTree(objectHash, "", recursive);
        }
        else if (program.is_subcommand_used("show-ref")) {
            auto references = GitRepository::repoDir("refs");
            for (const auto& [hash, refs] : GitCommands::getAll(references)) {
                for (const auto& ref : refs) {
                    std::cout << hash << ' ' << ref.string() << std::endl;
                }
            }
        }
        else if (program.is_subcommand_used("tag")) {
            auto& tagSubparser = program.at<argparse::ArgumentParser>("tag");
            bool isAssociative = tagSubparser.get<bool>("-a");
            bool tagHasName = tagSubparser.present("name").has_value();
            if (!isAssociative && !tagHasName) {
                auto tags = GitRepository::repoFile("refs", "tags");
                if (!tags.empty()) {
                    for (const auto& [_, tags] : GitCommands::getAll(tags)) {
                        for (const auto& tag : tags) {
                            std::cout << tag.filename().string() << std::endl;
                        }
                    }
                }
            }
            else if (tagHasName) {
                auto tagName = tagSubparser.get<std::string>("name");
                auto objectHash = GitObject::findObject(
                    tagSubparser.get<std::string>("object"));
                GitCommands::createTag(tagName, objectHash, isAssociative);
            }
            else {
                GENERATE_EXCEPTION("{}", tagSubparser.usage());
            }
        }
        else if (program.is_subcommand_used("rev-parse")) {
            auto& revSubParser =
                program.at<argparse::ArgumentParser>("rev-parse");
            auto objectName = revSubParser.get<std::string>("name");
            std::cout << GitObject::findObject(objectName, "") << std::endl;
        }
        else if (program.is_subcommand_used("ls-files")) {
            GitCommands::listFiles();
        }
        else if (program.is_subcommand_used("commit")) {
            auto& commitSubParser =
                program.at<argparse::ArgumentParser>("commit");
            std::string commitMessage =
                commitSubParser.present("-m")
                    ? commitSubParser.get<std::string>("-m")
                    : "Auto generated commit message";
            GitCommands::commit(commitMessage);
        }
        else if (program.is_subcommand_used("branch")) {
            auto& branchSubParser =
                program.at<argparse::ArgumentParser>("branch");
            if (branchSubParser.present("name")) {
                GitCommands::createBranch(
                    branchSubParser.get<std::string>("name"));
            }
            else {
                GitCommands::showBranches();
            }
        }
        else if (program.is_subcommand_used("checkout")) {
            auto commitToCheckoutTo =
                program.at<argparse::ArgumentParser>("checkout")
                    .get<std::string>("commit");
            GitCommands::checkout(commitToCheckoutTo);
        }
        else {
            GENERATE_EXCEPTION("{}", program.help().str());
        }
    }
    catch (const std::exception& myEx) {
        std::cerr << myEx.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
