#include "Zlib.hpp"
#include "../utilities/Common.hpp"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <fstream>

namespace Zlib {
namespace bio = boost::iostreams;

std::string compress(const std::string& data)
{
    std::istringstream origin(data);

    bio::filtering_istreambuf in;
    in.push(bio::zlib_compressor(bio::zlib::best_compression));
    in.push(origin);

    std::ostringstream compressed;
    bio::copy(in, compressed);
    return compressed.str();
}

void compress(const std::filesystem::path& filePath, const std::string& data)
{
    Utilities::writeToFile(filePath, compress(data));
}

std::string decompress(const std::string& data)
{
    std::istringstream compressed(data);

    bio::filtering_istreambuf in;
    in.push(bio::zlib_decompressor());
    in.push(compressed);

    std::ostringstream origin;
    bio::copy(in, origin);
    return origin.str();
}

std::string decompressFile(const std::filesystem::path& filePath)
{
    return decompress(Utilities::readFile(filePath));
}

}; // namespace Zlib