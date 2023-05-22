#include "Zlib.hpp"

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

std::string compressFile(const std::filesystem::path& filePath)
{
    return compress(readFile(filePath));
}
std::string decompressFile(const std::filesystem::path& filePath)
{
    return decompress(readFile(filePath));
}

std::string readFile(const std::filesystem::path& filePath)
{
    std::ifstream ifs(filePath.string(), std::ios::in | std::ios::binary);
    std::ostringstream data;
    data << ifs.rdbuf();
    return data.str();
}

void writeToFile(const std::filesystem::path& filePath, const std::string& data)
{
    std::ofstream ofs(filePath.string(), std::ios::out | std::ios::binary);
    ofs.write(data.c_str(), data.size());
}

}; // namespace Zlib