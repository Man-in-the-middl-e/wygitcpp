#pragma once

#include <fmt/core.h>
#include <boost/uuid/detail/sha1.hpp>

namespace Git {
#define GENERATE_EXCEPTION(MSG, PARAMETERS)                                    \
    throw std::runtime_error(fmt::format(MSG, PARAMETERS));
}; // namespace Git