#include "block_symbol.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "block/block.hpp"
#include "util/picture_load.hpp"
#include <algorithm>

namespace horizon {

UUID BlockSymbolPort::get_uuid_for_net(const UUID &net)
{
    static const UUID ns{"8fd9cbab-73f6-4778-befe-a9a3369e7f0d"};
    return UUID::UUID5(ns, net.get_bytes(), UUID::size);
}

BlockSymbolPort::BlockSymbolPort(const UUID &uu, const json &j)
    : uuid(uu), net(j.at("net").get<std::string>()), position(j["position"].get<std::vector<int64_t>>()),
      length(j["length"]), orientation(orientation_lut.lookup(j["orientation"]))
{
    if (j.count("name_orientation")) {
        name_orientation = pin_name_orientation_lut.lookup(j.at("name_orientation"));
    }
}

BlockSymbolPort::BlockSymbolPort(UUID uu) : uuid(uu)
{
}

Orientation BlockSymbolPort::get_orientation_for_placement(const Placement &pl) const
{
    return get_pin_orientation_for_placement(orientation, pl);
}

json BlockSymbolPort::serialize() const
{
    json j;
    j["position"] = position.as_array();
    j["length"] = length;
    j["net"] = (std::string)net;
    j["orientation"] = orientation_lut.lookup_reverse(orientation);
    j["name_orientation"] = pin_name_orientation_lut.lookup_reverse(name_orientation);
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
    if (j.count("pictures")) {
        const json &o = j["pictures"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            pictures.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
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

UUID BlockSymbolPort::get_uuid() const
{
    return uuid;
}

BlockSymbolPort *BlockSymbol::get_port_for_net(const UUID &net)
{
    auto uu = BlockSymbolPort::get_uuid_for_net(net);
    if (ports.count(uu)) {
        auto &p = ports.at(uu);
        assert(p.net == net);
        return &p;
    }
    return nullptr;
}


const BlockSymbolPort *BlockSymbol::get_port_for_net(const UUID &net) const
{
    auto uu = BlockSymbolPort::get_uuid_for_net(net);
    if (ports.count(uu)) {
        auto &p = ports.at(uu);
        assert(p.net == net);
        return &p;
    }
    return nullptr;
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
      texts(sym.texts), pictures(sym.pictures)
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
    pictures = sym.pictures;
    update_refs();
}

void BlockSymbol::expand()
{
    std::map<UUID, UUID> port_nets;
    for (const auto &[uu, port] : ports) {
        port_nets.emplace(uu, port.net);
    }
    for (const auto &[uu, net_uu] : port_nets) {
        if (block->nets.count(net_uu) && block->nets.at(net_uu).is_port) {
            const auto &net = block->nets.at(net_uu);
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

void BlockSymbol::create_template()
{
    const auto w = 1.25_mm * 12;
    const auto h = 1.25_mm * 8;
    {
        std::array<Junction *, 4> js;
        for (int i = 0; i < 4; i++) {
            auto uu = UUID::random();
            js.at(i) = &junctions.emplace(uu, uu).first->second;
        }
        js.at(0)->position = Coordi(-w / 2, h / 2);
        js.at(1)->position = Coordi(w / 2, h / 2);
        js.at(2)->position = Coordi(w / 2, -h / 2);
        js.at(3)->position = Coordi(-w / 2, -h / 2);
        for (int i = 0; i < 4; i++) {
            auto j = (i + 1) % 4;
            auto uu = UUID::random();
            auto &line = lines.emplace(uu, uu).first->second;
            line.from = js.at(i);
            line.to = js.at(j);
        }
    }
    {
        auto uu = UUID::random();
        auto &txt = texts.emplace(uu, uu).first->second;
        txt.placement.shift = Coordi(-w / 2, h / 2 + 1.25_mm);
        txt.text = "$REFDES";
    }
    {
        auto uu = UUID::random();
        auto &txt = texts.emplace(uu, uu).first->second;
        txt.placement.shift = Coordi(-w / 2, -h / 2 - 1.25_mm);
        txt.text = "$NAME";
    }
}

void BlockSymbol::load_pictures(const std::string &dir)
{
    pictures_load({&pictures}, dir, "sym");
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
    j["pictures"] = json::object();
    for (const auto &it : pictures) {
        j["pictures"][(std::string)it.first] = it.second.serialize();
    }

    return j;
}
} // namespace horizon
