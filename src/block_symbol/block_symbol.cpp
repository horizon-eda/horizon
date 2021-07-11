#include "block_symbol.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "block/block.hpp"
#include <algorithm>

namespace horizon {

BlockSymbolPort::BlockSymbolPort(const UUID &uu, const json &j)
    : uuid(uu), position(j["position"].get<std::vector<int64_t>>()), length(j["length"]),
      orientation(orientation_lut.lookup(j["orientation"]))
{
}

BlockSymbolPort::BlockSymbolPort(UUID uu) : uuid(uu)
{
}

static const std::map<Orientation, Orientation> omap_90 = {
        {Orientation::LEFT, Orientation::DOWN},
        {Orientation::UP, Orientation::LEFT},
        {Orientation::RIGHT, Orientation::UP},
        {Orientation::DOWN, Orientation::RIGHT},
};
static const std::map<Orientation, Orientation> omap_180 = {
        {Orientation::LEFT, Orientation::RIGHT},
        {Orientation::UP, Orientation::DOWN},
        {Orientation::RIGHT, Orientation::LEFT},
        {Orientation::DOWN, Orientation::UP},
};
static const std::map<Orientation, Orientation> omap_270 = {
        {Orientation::LEFT, Orientation::UP},
        {Orientation::UP, Orientation::RIGHT},
        {Orientation::RIGHT, Orientation::DOWN},
        {Orientation::DOWN, Orientation::LEFT},
};
static const std::map<Orientation, Orientation> omap_mirror = {
        {Orientation::LEFT, Orientation::RIGHT},
        {Orientation::UP, Orientation::UP},
        {Orientation::RIGHT, Orientation::LEFT},
        {Orientation::DOWN, Orientation::DOWN},
};

Orientation BlockSymbolPort::get_orientation_for_placement(const Placement &pl) const
{
    Orientation pin_orientation = orientation;
    auto angle = pl.get_angle();
    if (angle == 16384) {
        pin_orientation = omap_90.at(pin_orientation);
    }
    if (angle == 32768) {
        pin_orientation = omap_180.at(pin_orientation);
    }
    if (angle == 49152) {
        pin_orientation = omap_270.at(pin_orientation);
    }
    if (pl.mirror) {
        pin_orientation = omap_mirror.at(pin_orientation);
    }
    return pin_orientation;
}

json BlockSymbolPort::serialize() const
{
    json j;
    j["position"] = position.as_array();
    j["length"] = length;
    j["orientation"] = orientation_lut.lookup_reverse(orientation);
    return j;
}

BlockSymbol::BlockSymbol(const UUID &uu, const json &j, const Block &bl) : uuid(uu), block(&bl)
{
    if (j.count("junctions")) {
        const json &o = j["junctions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            junctions.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
    }
    if (j.count("lines")) {
        const json &o = j["lines"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            lines.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                          std::forward_as_tuple(u, it.value(), *this));
        }
    }
    if (j.count("ports")) {
        const json &o = j["ports"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            ports.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
    }
    if (j.count("arcs")) {
        const json &o = j["arcs"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            arcs.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                         std::forward_as_tuple(u, it.value(), *this));
        }
    }
    if (j.count("texts")) {
        const json &o = j["texts"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            texts.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
    }
}

BlockSymbol::BlockSymbol(const UUID &uu, const Block &b) : uuid(uu), block(&b)
{
}

BlockSymbol BlockSymbol::new_from_file(const std::string &filename, const Block &bl)
{
    auto j = load_json_from_file(filename);
    return BlockSymbol(UUID(j["uuid"].get<std::string>()), j, bl);
}

Junction *BlockSymbol::get_junction(const UUID &uu)
{
    return junctions.count(uu) ? &junctions.at(uu) : nullptr;
}
BlockSymbolPort *BlockSymbol::get_block_symbol_port(const UUID &uu)
{
    return ports.count(uu) ? &ports.at(uu) : nullptr;
}

UUID BlockSymbolPort::get_uuid() const
{
    return uuid;
}

void BlockSymbol::update_refs()
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

BlockSymbol::BlockSymbol(const BlockSymbol &sym)
    : uuid(sym.uuid), block(sym.block), ports(sym.ports), junctions(sym.junctions), lines(sym.lines), arcs(sym.arcs),
      texts(sym.texts)
{
    update_refs();
}

void BlockSymbol::operator=(BlockSymbol const &sym)
{
    uuid = sym.uuid;
    block = sym.block;
    ports = sym.ports;
    junctions = sym.junctions;
    lines = sym.lines;
    arcs = sym.arcs;
    texts = sym.texts;
    update_refs();
}

void BlockSymbol::expand()
{
    std::vector<UUID> keys;
    keys.reserve(ports.size());
    for (const auto &it : ports) {
        keys.push_back(it.first);
    }
    for (const auto &uu : keys) {
        if (block->nets.count(uu) && block->nets.at(uu).is_port) {
            const auto &net = block->nets.at(uu);
            auto &p = ports.at(uu);
            if (net.is_port) {
                p.name = net.name;
                p.direction = net.port_direction;
            }
        }
        else {
            ports.erase(uu);
        }
    }
}

std::pair<Coordi, Coordi> BlockSymbol::get_bbox(bool all) const
{
    Coordi a;
    Coordi b;
    for (const auto &it : junctions) {
        a = Coordi::min(a, it.second.position);
        b = Coordi::max(b, it.second.position);
    }
    for (const auto &it : ports) {
        a = Coordi::min(a, it.second.position);
        b = Coordi::max(b, it.second.position);
    }
    if (all) {
        for (const auto &it : texts) {
            a = Coordi::min(a, it.second.placement.shift);
            b = Coordi::max(b, it.second.placement.shift);
        }
    }
    return std::make_pair(a, b);
}

json BlockSymbol::serialize() const
{
    json j;
    j["type"] = "block_symbol";
    j["uuid"] = (std::string)uuid;
    j["block"] = (std::string)block->uuid;
    j["junctions"] = json::object();
    for (const auto &it : junctions) {
        j["junctions"][(std::string)it.first] = it.second.serialize();
    }
    j["ports"] = json::object();
    for (const auto &it : ports) {
        j["ports"][(std::string)it.first] = it.second.serialize();
    }
    j["lines"] = json::object();
    for (const auto &it : lines) {
        j["lines"][(std::string)it.first] = it.second.serialize();
    }
    j["arcs"] = json::object();
    for (const auto &it : arcs) {
        j["arcs"][(std::string)it.first] = it.second.serialize();
    }
    j["texts"] = json::object();
    for (const auto &it : texts) {
        j["texts"][(std::string)it.first] = it.second.serialize();
    }

    return j;
}
} // namespace horizon
