#pragma once
#include "common/common.hpp"
#include <string>

namespace horizon {
class Warning {
public:
    Warning(const Coordi &c, const std::string &t) : position(c), text(t)
    {
    }
    Coordi position;
    std::string text;
};
} // namespace horizon
