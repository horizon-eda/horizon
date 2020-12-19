#pragma once
#include "junction.hpp"
#include "line.hpp"
#include "arc.hpp"

namespace horizon {
class JunctionUtil {
public:
    static void update(std::map<UUID, Line> &lines);
    static void update(std::map<UUID, Arc> &arc);
};
} // namespace horizon
