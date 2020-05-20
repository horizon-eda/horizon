#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "common.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
using json = nlohmann::json;

class Dimension {
public:
    Dimension(const UUID &uu, const json &j);
    Dimension(const UUID &uu);

    UUID uuid;
    Coordi p0;
    Coordi p1;
    int64_t label_distance = 3_mm;
    uint64_t label_size = 1.5_mm;

    enum class Mode { HORIZONTAL, VERTICAL, DISTANCE };
    Mode mode = Mode::DISTANCE;

    int64_t project(const Coordi &c) const;
    int64_t get_length() const;

    json serialize() const;
};
} // namespace horizon
