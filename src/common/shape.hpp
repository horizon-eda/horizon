#pragma once
#include "util/uuid.hpp"
#include "common.hpp"
#include <nlohmann/json_fwd.hpp>
#include "util/placement.hpp"
#include "polygon.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

/**
 * For commonly used Pad shapes
 */
class Shape {
public:
    Shape(const UUID &uu, const json &j);
    Shape(const UUID &uu);

    UUID uuid;
    Placement placement;
    int layer = 0;
    std::string parameter_class;

    enum class Form { CIRCLE, RECTANGLE, OBROUND };
    Form form = Form::CIRCLE;
    std::vector<int64_t> params;

    Polygon to_polygon() const;
    std::pair<Coordi, Coordi> get_bbox() const;

    UUID get_uuid() const;

    json serialize() const;
};
} // namespace horizon
