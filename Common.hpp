#pragma once

#include <fmt/core.h>

namespace Git {
#define GENERATE_EXCEPTION(MSG, PARAMETERS) throw std::runtime_error(fmt::format(MSG, PARAMETERS));
};