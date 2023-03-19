#pragma once
#include "util/uuid.hpp"
#include "common.hpp"
#include "nlohmann/json_fwd.hpp"
#include "polygon.hpp"
#include "util/placement.hpp"


namespace horizon {
using json = nlohmann::json;

/**
 * A hole with diameter and position, that's it.
 */
class Hole {
public:
    Hole(const UUID &uu, const json &j);
    Hole(const UUID &uu);

    UUID uuid;
    Placement placement;
    uint64_t diameter = 0.5_mm;
    uint64_t length = 0.5_mm;
    std::string parameter_class;

    /**
     * true if this hole is PTH, false if NPTH.
     * Used by the gerber exporter.
     */
    bool plated = false;

    enum class Shape { ROUND, SLOT };
    Shape shape = Shape::ROUND;

    LayerRange span;

    Polygon to_polygon() const;
    std::pair<Coordi, Coordi> get_bbox() const;

    UUID get_uuid() const;

    // not stored
    json serialize() const;
};
} // namespace horizon
