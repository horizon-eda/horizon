#include "decal.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "board/board_layers.hpp"
#include "util/bbox_accumulator.hpp"

namespace horizon {

static const unsigned int app_version = 0;

unsigned int Decal::get_app_version()
{
    return app_version;
}

Decal::Decal(const UUID &uu, const json &j) : uuid(uu), name(j.value("name", "")), version(app_version, j)
{
    check_object_type(j, ObjectType::DECAL);
    version.check(ObjectType::DECAL, name, uuid);
    if (j.count("junctions")) {
        const json &o = j["junctions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            junctions.emplace(std::make_pair(u, Junction(u, it.value())));
        }
    }
    if (j.count("lines")) {
        const json &o = j["lines"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            lines.emplace(std::make_pair(u, Line(u, it.value(), *this)));
        }
    }
    if (j.count("arcs")) {
        const json &o = j["arcs"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            arcs.emplace(std::make_pair(u, Arc(u, it.value(), *this)));
        }
    }
    if (j.count("texts")) {
        const json &o = j["texts"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            texts.emplace(std::make_pair(u, Text(u, it.value())));
        }
    }
    if (j.count("polygons")) {
        const json &o = j["polygons"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            polygons.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
    }
    map_erase_if(polygons, [](const auto &a) { return a.second.vertices.size() == 0; });
}

Decal::Decal(const UUID &uu) : uuid(uu), version(app_version)
{
}

Decal Decal::new_from_file(const std::string &filename)
{
    json j = load_json_from_file(filename);
    return Decal(UUID(j.at("uuid").get<std::string>()), j);
}

Junction *Decal::get_junction(const UUID &uu)
{
    return junctions.count(uu) ? &junctions.at(uu) : nullptr;
}

void Decal::update_refs()
{
    for (auto &it : lines) {
        auto &line = it.second;
        line.to = &junctions.at(line.to.uuid);
        line.from = &junctions.at(line.from.uuid);
    }
    for (auto &it : arcs) {
        auto &arc = it.second;
        arc.to = &junctions.at(arc.to.uuid);
        arc.from = &junctions.at(arc.from.uuid);
        arc.center = &junctions.at(arc.center.uuid);
    }
}

Decal::Decal(const Decal &fr)
    : uuid(fr.uuid), name(fr.name), junctions(fr.junctions), polygons(fr.polygons), lines(fr.lines), arcs(fr.arcs),
      texts(fr.texts), version(fr.version)
{
    update_refs();
}

void Decal::operator=(Decal const &fr)
{
    uuid = fr.uuid;
    name = fr.name;
    junctions = fr.junctions;
    lines = fr.lines;
    arcs = fr.arcs;
    texts = fr.texts;
    polygons = fr.polygons;
    version = fr.version;
    update_refs();
}

UUID Decal::get_uuid() const
{
    return uuid;
}

static std::pair<Coordi, Coordi> get_line_bb(const Line &li)
{
    Coordi a = Coordi::min(li.from->position, li.to->position);
    Coordi b = Coordi::max(li.from->position, li.to->position);
    Coordi w(li.width / 2, li.width / 2);
    return {a - w, b + w};
}

std::pair<Coordi, Coordi> Decal::get_bbox() const
{
    BBoxAccumulator<Coordi::type> acc;
    for (const auto &it : lines) {
        acc.accumulate(get_line_bb(it.second));
    }
    for (const auto &it : polygons) {
        acc.accumulate(it.second.get_bbox());
    }
    return acc.get_or_0();
}

json Decal::serialize() const
{
    json j;
    version.serialize(j);
    j["type"] = "decal";
    j["name"] = name;
    j["uuid"] = (std::string)uuid;
    j["junctions"] = json::object();
    for (const auto &it : junctions) {
        j["junctions"][(std::string)it.first] = it.second.serialize();
    }
    j["lines"] = json::object();
    for (const auto &it : lines) {
        j["lines"][(std::string)it.first] = it.second.serialize();
    }
    j["arcs"] = json::object();
    for (const auto &it : arcs) {
        j["arcs"][(std::string)it.first] = it.second.serialize();
    }
    j["polygons"] = json::object();
    for (const auto &it : polygons) {
        j["polygons"][(std::string)it.first] = it.second.serialize();
    }
    j["texts"] = json::object();
    for (const auto &it : texts) {
        j["texts"][(std::string)it.first] = it.second.serialize();
    }
    return j;
}

static std::map<int, Layer> decal_layers;

const std::map<int, Layer> &Decal::get_layers() const
{
    if (decal_layers.size() == 0) {
        auto add_layer = [](int n, bool r = false, bool c = false) {
            decal_layers.emplace(std::piecewise_construct, std::forward_as_tuple(n),
                                 std::forward_as_tuple(n, BoardLayers::get_layer_name(n), r, c));
        };
        add_layer(BoardLayers::TOP_ASSEMBLY);
        add_layer(BoardLayers::TOP_SILKSCREEN);
        add_layer(BoardLayers::TOP_MASK);
        add_layer(BoardLayers::TOP_COPPER, false, true);
    }
    return decal_layers;
}


} // namespace horizon
