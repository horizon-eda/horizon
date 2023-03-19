#include "hole.hpp"
#include "lut.hpp"
#include "nlohmann/json.hpp"
#include "board/board_layers.hpp"

namespace horizon {
using json = nlohmann::json;

static const LutEnumStr<Hole::Shape> shape_lut = {
        {"round", Hole::Shape::ROUND},
        {"slot", Hole::Shape::SLOT},
};

Hole::Hole(const UUID &uu, const json &j)
    : uuid(uu), placement(j.at("placement")), diameter(j.at("diameter").get<uint64_t>()),
      length(j.at("length").get<uint64_t>()), parameter_class(j.value("parameter_class", "")),
      plated(j.at("plated").get<bool>()), shape(shape_lut.lookup(j.value("shape", "round"))),
      span(BoardLayers::layer_range_through)
{
    if (j.count("span"))
        span = LayerRange(j.at("span"));
}

UUID Hole::get_uuid() const
{
    return uuid;
}

Polygon Hole::to_polygon() const
{
    auto uu = UUID();
    Polygon poly(uu);
    poly.layer = 10000;
    switch (shape) {
    case Shape::ROUND: {
        int64_t r = diameter / 2;
        Polygon::Vertex *v;
        v = poly.append_vertex({r, 0});
        v->type = Polygon::Vertex::Type::ARC;
        v->arc_reverse = true;
        poly.append_vertex({r, 0});
    } break;

    case Shape::SLOT: {
        int64_t h = diameter / 2;
        int64_t w = length / 2;
        w = std::max(w - h, (int64_t)0);

        Polygon::Vertex *v;
        v = poly.append_vertex({w, h});
        v->type = Polygon::Vertex::Type::ARC;
        v->arc_center = {w, 0};
        v->arc_reverse = true;
        poly.append_vertex({w, -h});
        v = poly.append_vertex({-w, -h});
        v->type = Polygon::Vertex::Type::ARC;
        v->arc_center = {-w, 0};
        v->arc_reverse = true;
        poly.append_vertex({-w, h});

    } break;
    }
    for (auto &it : poly.vertices) {
        it.position = placement.transform(it.position);
        it.arc_center = placement.transform(it.arc_center);
    }
    return poly;
}

std::pair<Coordi, Coordi> Hole::get_bbox() const
{
    switch (shape) {
    case Shape::SLOT: {
        const auto w = length / 2;
        const auto h = diameter / 2;
        return {Coordi(-w, -h), Coordi(w, h)};
    } break;

    case Shape::ROUND: {
        const auto r = diameter / 2;
        return {Coordi(-r, -r), Coordi(r, r)};
    } break;
    }
    return {Coordi(), Coordi()};
}

Hole::Hole(const UUID &uu) : uuid(uu), span(BoardLayers::layer_range_through)
{
}

json Hole::serialize() const
{
    json j;
    j["placement"] = placement.serialize();
    j["diameter"] = diameter;
    j["length"] = length;
    j["shape"] = shape_lut.lookup_reverse(shape);
    j["plated"] = plated;
    j["parameter_class"] = parameter_class;
    if (span != BoardLayers::layer_range_through)
        j["span"] = span.serialize();
    return j;
}
} // namespace horizon
