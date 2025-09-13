#include "polygon.hpp"
#include "lut.hpp"
#include <nlohmann/json.hpp>
#include "util/geom_util.hpp"
#include "util/bbox_accumulator.hpp"
#include "util/polygon_arc_removal_proxy.hpp"

namespace horizon {

static const LutEnumStr<Polygon::Vertex::Type> type_lut = {{"line", Polygon::Vertex::Type::LINE},
                                                           {"arc", Polygon::Vertex::Type::ARC}};

Polygon::Vertex::Vertex(const json &j)
    : type(type_lut.lookup(j.at("type").get<std::string>())), position(j.at("position").get<std::vector<int64_t>>()),
      arc_center(j.at("arc_center").get<std::vector<int64_t>>()), arc_reverse(j.value("arc_reverse", false))
{
}

Polygon::Vertex::Vertex(const Coordi &c) : position(c)
{
}

json Polygon::Vertex::serialize() const
{
    json j;
    j["type"] = type_lut.lookup_reverse(type);
    j["position"] = position.as_array();
    j["arc_center"] = arc_center.as_array();
    j["arc_reverse"] = arc_reverse;
    return j;
}

Polygon::Polygon(const UUID &uu, const json &j)
    : uuid(uu), layer(j.value("layer", 0)), parameter_class(j.value("parameter_class", ""))
{
    {
        const json &o = j["vertices"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            vertices.emplace_back(it.value());
        }
    }
}

Polygon::Polygon(const UUID &uu) : uuid(uu)
{
}

UUID Polygon::get_uuid() const
{
    return uuid;
}

Polygon Polygon::remove_arcs(unsigned int precision) const
{
    Polygon out(uuid);
    out.layer = layer;
    out.usage = usage;
    if (!has_arcs()) {
        out.vertices = vertices;
        return out;
    }

    for (auto it = vertices.cbegin(); it < vertices.cend(); it++) {
        if (it->type == Polygon::Vertex::Type::LINE) {
            out.vertices.emplace_back(*it);
        }
        else {
            out.append_vertex(it->position);
            auto it_next = it + 1;
            if (it_next == vertices.cend()) {
                it_next = vertices.cbegin();
            }
            const Coordd a(it->position);
            const Coordd b(it_next->position);
            const Coordd c = project_onto_perp_bisector(a, b, it->arc_center);
            double radius0 = (a - c).mag();
            double radius1 = (b - c).mag();
            Color co(1, 1, 0);
            double a0 = (a - c).angle();
            double a1 = (b - c).angle();

            unsigned int segments = precision;
            if (a0 < 0) {
                a0 += 2 * M_PI;
            }
            if (a1 < 0) {
                a1 += 2 * M_PI;
            }
            double dphi = a1 - a0;
            if (dphi < 0) {
                dphi += 2 * M_PI;
            }
            if (it->arc_reverse) {
                dphi -= 2 * M_PI;
            }
            dphi /= segments;

            float dr = radius1 - radius0;
            dr /= segments;
            segments--;
            while (segments--) {
                a0 += dphi;
                auto p1f = c + Coordd::euler(radius0, a0);
                Coordi p1(p1f.x, p1f.y);
                out.append_vertex(p1);
                radius0 += dr;
            }
        }
    }

    return out;
}

bool Polygon::has_arcs() const
{
    for (const auto &it : vertices) {
        if (it.type == Polygon::Vertex::Type::ARC)
            return true;
    }
    return false;
}

bool Polygon::is_valid() const
{
    if (has_arcs())
        return vertices.size() >= 2;
    else
        return vertices.size() >= 3;
}

Polygon::Vertex *Polygon::append_vertex(const Coordi &pos)
{
    vertices.emplace_back();
    vertices.back().position = pos;
    return &vertices.back();
}

std::pair<unsigned int, unsigned int> Polygon::get_vertices_for_edge(unsigned int edge)
{
    return {edge, (edge + 1) % vertices.size()};
}

const Polygon::Vertex &Polygon::get_vertex(int edge) const
{
    while (edge < 0)
        edge += vertices.size();
    return vertices.at(edge % vertices.size());
}

Polygon::Vertex &Polygon::get_vertex(int edge)
{
    return const_cast<Polygon::Vertex &>(const_cast<const Polygon *>(this)->get_vertex(edge));
}

std::pair<Coordi, Coordi> Polygon::get_bbox() const
{
    PolygonArcRemovalProxy proxy(*this, 8);
    const auto &poly = proxy.get();
    BBoxAccumulator<Coordi::type> acc;
    for (const auto &v : poly.vertices) {
        acc.accumulate(v.position);
    }
    return acc.get();
}

json Polygon::serialize() const
{
    json j;
    j["layer"] = layer;
    j["parameter_class"] = parameter_class;
    j["vertices"] = json::array();
    for (const auto &it : vertices) {
        j["vertices"].push_back(it.serialize());
    }

    return j;
}

bool Polygon::is_cw() const
{
    return !is_ccw();
}

bool Polygon::is_ccw() const
{
    PolygonArcRemovalProxy prx(*this, 2);
    const auto &vs = prx.get().vertices;
    // adapted from ClipperLib::Area
    int size = (int)vs.size();
    if (size < 3)
        return false;

    double a = 0;
    for (int i = 0, j = size - 1; i < size; ++i) {
        a += ((double)vs.at(j).position.x + vs.at(i).position.x) * ((double)vs.at(j).position.y - vs.at(i).position.y);
        j = i;
    }
    return a < 0;
}

void Polygon::reverse()
{
    std::reverse(vertices.begin(), vertices.end());

    // roll arcs type by one
    for (size_t i = 0; i < (vertices.size() - 1); i++) {
        std::swap(vertices.at(i).type, vertices.at(i + 1).type);
        std::swap(vertices.at(i).arc_center, vertices.at(i + 1).arc_center);
        std::swap(vertices.at(i).arc_reverse, vertices.at(i + 1).arc_reverse);
    }
    for (auto &it : vertices) {
        it.arc_reverse = !it.arc_reverse;
    }
}

bool Polygon::is_rect() const
{
    if (vertices.size() != 4)
        return false;
    if (has_arcs())
        return false;
    for (size_t i = 0; i < 4; i++) {
        const auto &p0 = get_vertex(i).position;
        const auto &p1 = get_vertex(i + 1).position;
        const auto &p2 = get_vertex(i + 2).position;
        const auto v0 = p1 - p0;
        const auto v1 = p2 - p1;
        if (v0.dot(v1) != 0)
            return false;
    }
    return true;
}

} // namespace horizon
