#include "canvas.hpp"
#include "common/polygon.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {
void Canvas::img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr)
{
    if (!img_mode)
        return;
    UUID uu;
    Polygon poly(uu);
    poly.layer = layer;
    auto v = p1 - p0;
    Coord<double> vn = v;
    if (vn.mag_sq() > 0) {
        vn = vn / sqrt(vn.mag_sq());
        vn *= width / 2;
    }
    else {
        vn = {(double)width / 2, 0};
    }
    Coordi vni(-vn.y, vn.x);
    poly.vertices.emplace_back(p0 + vni);
    auto &a0 = poly.vertices.back();
    a0.type = Polygon::Vertex::Type::ARC;
    a0.arc_center = p0;
    poly.vertices.emplace_back(p0 - vni);

    poly.vertices.emplace_back(p1 - vni);
    auto &a1 = poly.vertices.back();
    a1.type = Polygon::Vertex::Type::ARC;
    a1.arc_center = p1;
    poly.vertices.emplace_back(p1 + vni);

    // poly.vertices.push_back(p0+vn, p0-vn);

    auto polyr = poly.remove_arcs();

    img_polygon(polyr, tr);
}

} // namespace horizon
