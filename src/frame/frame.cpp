#include "frame.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {


Frame::Frame(const UUID &uu, const json &j)
    : uuid(uu), name(j.value("name", "")), width(j.value("width", 297_mm)), height(j.value("height", 210_mm))
{
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

Frame::Frame(const UUID &uu) : uuid(uu)
{
}

Frame Frame::new_from_file(const std::string &filename)
{
    json j = load_json_from_file(filename);
    return Frame(UUID(j.at("uuid").get<std::string>()), j);
}

Junction *Frame::get_junction(const UUID &uu)
{
    return junctions.count(uu) ? &junctions.at(uu) : nullptr;
}

void Frame::update_refs()
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

Frame::Frame(const Frame &fr)
    : uuid(fr.uuid), name(fr.name), junctions(fr.junctions), lines(fr.lines), arcs(fr.arcs), texts(fr.texts),
      polygons(fr.polygons), width(fr.width), height(fr.height)
{
    update_refs();
}

void Frame::operator=(Frame const &fr)
{
    uuid = fr.uuid;
    name = fr.name;
    junctions = fr.junctions;
    lines = fr.lines;
    arcs = fr.arcs;
    texts = fr.texts;
    polygons = fr.polygons;
    width = fr.width;
    height = fr.height;
    update_refs();
}

void Frame::expand()
{
}

UUID Frame::get_uuid() const
{
    return uuid;
}

std::pair<Coordi, Coordi> Frame::get_bbox(bool all) const
{
    return {Coordi(), Coordi(width, height)};
}

json Frame::serialize() const
{
    json j;
    j["type"] = "frame";
    j["name"] = name;
    j["uuid"] = (std::string)uuid;
    j["width"] = width;
    j["height"] = height;
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
} // namespace horizon
