#include "GitObjectsFactory.hpp"

#include "../utilities/Zlib.hpp"

namespace Git {

std::unique_ptr<GitObject>
GitObjectFactory::create(const std::string& format,
                         const ObjectData& objectData)
{
    if (format == "commit") {
        return createObject<GitCommit>(objectData);
    }
    else if (format == "tree") {
        return createObject<GitTree>(objectData);
    }
    else if (format == "tag") {
        return createObject<GitTag>(objectData);
    }
    else if (format == "blob") {
        return createObject<GitBlob>(objectData);
    }
    GENERATE_EXCEPTION("Wrong Git Object format: {}", format);
}

std::unique_ptr<GitObject> GitObjectFactory::read(const GitHash& objectHash)
{
    auto path = GitRepository::repoFile("objects", objectHash.directoryName(),
                                        objectHash.fileName());

    auto objectContent = Zlib::decompressFile(path);
    /*
        |object type|| ||size||0|
        |content ...|
    */
    auto formatEnds = objectContent.find_first_of(' ');
    auto format = objectContent.substr(0, formatEnds);

    auto sizeEnds = objectContent.find_first_of('\0', formatEnds);
    auto size = std::stoi(objectContent.substr(formatEnds, sizeEnds));

    if (size != objectContent.size() - sizeEnds - 1) {
        GENERATE_EXCEPTION("Malformed object: {}", objectHash.data());
    }

    auto objectData = ObjectData(objectContent.substr(sizeEnds + 1));
    return GitObjectFactory::create(format, objectData);
}
}; // namespace Git