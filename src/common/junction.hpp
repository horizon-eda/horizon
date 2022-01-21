#pragma once
#include "util/uuid.hpp"
#include "common.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/layer_range.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

/**
 * A Junction is a point in 2D-Space.
 * A Junction is referenced by Line, Arc, LineNet, etc.\ for storing
 * coordinates.
 * This allows for actually storing Line connections instead of relying
 * on coincident coordinates.
 * When used on a Board or a Sheet, a Junction may get assigned a Net
 * or a Bus and a net segment.
 */
class Junction {
public:
    Junction(const UUID &uu, const json &j);
    Junction(const UUID &uu);

    UUID uuid;
    Coord<int64_t> position;

    UUID get_uuid() const;

    // not stored
    LayerRange layer = 10000;
    std::vector<UUID> connected_lines;
    std::vector<UUID> connected_arcs;
    virtual bool only_lines_arcs_connected() const;
    void clear();

    json serialize() const;
    virtual ~Junction()
    {
    }
};
} // namespace horizon
