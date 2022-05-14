#include "canvas.hpp"
#include "common/polygon.hpp"
#include <algorithm>
#include "util/geom_util.hpp"

namespace horizon {
void Canvas::img_line(const Coordi &p0, const Coordi &p1, uint64_t width, int layer, bool tr)
{
    if (!img_mode)
        return;
    if (!img_layer_is_visible(layer))
        return;
    width = std::max(width, static_cast<uint64_t>(.001_mm));
    UUID uu;
    Polygon poly(uu);
    poly.layer = layer;
    auto v = p1 - p0;
    Coord<double> vn = v;
    if (vn.mag_sq() > 0) {
        vn = vn.normalize() * (width / 2);
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

void Canvas::img_arc(const Coordi &from, const Coordi &to, const Coordi &center, const uint64_t width, int layer)
{
    Coordi c = project_onto_perp_bisector(from, to, center).to_coordi();
    const auto radius0 = (c - from).magd();
    const auto a0 = c2pi((from - c).angle());
    const auto a1 = c2pi((to - c).angle());
    unsigned int segments = 64;

    float dphi = c2pi(a1 - a0);
    dphi /= segments;
    float a = a0;
    while (segments--) {
        const auto p0 = c + Coordd::euler(radius0, a).to_coordi();
        const auto p1 = c + Coordd::euler(radius0, a + dphi).to_coordi();
        img_line(p0, p1, width, layer, true);
        a += dphi;
    }
}

} // namespace horizon
