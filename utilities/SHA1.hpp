#pragma once

#include <string>

#include "../git_objects/GitHash.hpp"

namespace Utilities {
class SHA1 {
  public:
    static GitHash computeHash(const std::string& data);

  private:
    SHA1() = delete;

    static std::string generateHash(const std::string& data);
    static std::string makeHashReadable(const std::string& hash);
};

};

using SHA1 = Utilities::SHA1;