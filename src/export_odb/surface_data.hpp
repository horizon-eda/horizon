#pragma once
#include <string>
#include <ostream>
#include <type_traits>
#include "attributes.hpp"
#include <map>
#include <vector>
#include "common/common.hpp"
#include "util/placement.hpp"

namespace horizon {
class Polygon;
} // namespace horizon

namespace horizon::ODB {

class SurfaceData {
public:
    class SurfaceLine {
    public:
        enum class Direction { CW, CCW };
        enum class Type { SEGMENT, ARC };

        SurfaceLine(const Coordi &c) : end(c)
        {
        }
        SurfaceLine(const Coordi &e, const Coordi &c, Direction d) : end(e), type(Type::ARC), center(c), direction(d)
        {
        }

        Coordi end;
        Type type = Type::SEGMENT;


        Coordi center;
        Direction direction;
    };

    void write(std::ostream &ost) const;

    void append_polygon(const Polygon &poly, const Placement &transform = Placement());
    void append_polygon_auto_orientation(const Polygon &poly, const Placement &transform = Placement());


    // first one is contour (island) oriented clockwise
    // remainder are holes oriented counter clockwise
    std::vector<std::vector<SurfaceLine>> lines;
};

} // namespace horizon::ODB
