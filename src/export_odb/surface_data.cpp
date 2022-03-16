#include "surface_data.hpp"
#include "common/polygon.hpp"
#include "util/once.hpp"
#include "odb_util.hpp"

namespace horizon::ODB {
void SurfaceData::append_polygon(const Polygon &poly, const Placement &transform)
{
    lines.emplace_back();
    auto &contour = lines.back();
    contour.reserve(poly.vertices.size());
    for (size_t i = 0; i < poly.vertices.size(); i++) {
        const auto &v = poly.vertices.at(i);
        const auto &v_next = poly.get_vertex(i - 1);
        using D = SurfaceLine::Direction;
        if (v_next.type == Polygon::Vertex::Type::LINE)
            contour.emplace_back(transform.transform(v.position));
        else
            contour.emplace_back(transform.transform(v.position), transform.transform(v_next.arc_center),
                                 (v_next.arc_reverse != transform.mirror) ? D::CW : D::CCW);
    }
}

void SurfaceData::append_polygon_auto_orientation(const Polygon &poly, const Placement &transform)
{
    const bool need_cw = lines.size() == 0; // first one is cw
    const bool need_reverse = (poly.is_ccw() != transform.mirror) != need_cw;
    if (need_reverse) {
        auto poly2 = poly;
        poly2.reverse();
        append_polygon(poly2, transform);
    }
    else {
        append_polygon(poly, transform);
    }
}


void SurfaceData::write(std::ostream &ost) const
{
    Once is_island;
    for (const auto &contour : lines) {
        ost << "OB " << contour.back().end << " ";
        if (is_island())
            ost << "I";
        else
            ost << "H";
        ost << endl;
        for (const auto &line : contour) {
            if (line.type == SurfaceLine::Type::SEGMENT)
                ost << "OS " << line.end << endl;
            else
                ost << "OC " << line.end << " " << line.center << " "
                    << (line.direction == SurfaceLine::Direction::CW ? "Y" : "N") << endl;
        }
        ost << "OE" << endl;
    }
}
} // namespace horizon::ODB
