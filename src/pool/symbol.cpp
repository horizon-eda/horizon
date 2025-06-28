#include "symbol.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/lut.hpp"
#include "ipool.hpp"
#include "util/util.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include "util/bbox_accumulator.hpp"
#include "common/junction_util.hpp"

namespace horizon {

static const LutEnumStr<SymbolPin::Decoration::Driver> decoration_driver_lut = {
        {"default", SymbolPin::Decoration::Driver::DEFAULT},
        {"open_collector", SymbolPin::Decoration::Driver::OPEN_COLLECTOR},
        {"open_collector_pullup", SymbolPin::Decoration::Driver::OPEN_COLLECTOR_PULLUP},
        {"open_emitter", SymbolPin::Decoration::Driver::OPEN_EMITTER},
        {"open_emitter_pulldown", SymbolPin::Decoration::Driver::OPEN_EMITTER_PULLDOWN},
        {"tristate", SymbolPin::Decoration::Driver::TRISTATE},
};

SymbolPin::Decoration::Decoration()
{
}
SymbolPin::Decoration::Decoration(const json &j)
    : dot(j.at("dot")), clock(j.at("clock")), schmitt(j.at("schmitt")),
      driver(decoration_driver_lut.lookup(j.at("driver")))
{
}

json SymbolPin::Decoration::serialize() const
{
    json j;
    j["dot"] = dot;
    j["clock"] = clock;
    j["schmitt"] = schmitt;
    j["driver"] = decoration_driver_lut.lookup_reverse(driver);
    return j;
}

SymbolPin::SymbolPin(const UUID &uu, const json &j)
    : uuid(uu), position(j.at("position").get<std::vector<int64_t>>()), length(j.at("length")),
      name_visible(j.value("name_visible", true)), pad_visible(j.value("pad_visible", true)),
      orientation(orientation_lut.lookup(j.at("orientation")))
{
    if (j.count("decoration")) {
        decoration = Decoration(j.at("decoration"));
    }
    if (j.count("keep_horizontal")) {
        if (j.at("keep_horizontal").get<bool>()) {
            name_orientation = NameOrientation::HORIZONTAL;
        }
    }
    else if (j.count("name_orientation")) {
        name_orientation = pin_name_orientation_lut.lookup(j.at("name_orientation"));
    }
}

SymbolPin::SymbolPin(UUID uu) : uuid(uu)
{
}

Orientation SymbolPin::get_orientation_for_placement(const Placement &pl) const
{
    return get_pin_orientation_for_placement(orientation, pl);
}

json SymbolPin::serialize() const
{
    json j;
    j["position"] = position.as_array();
    j["length"] = length;
    j["orientation"] = orientation_lut.lookup_reverse(orientation);
    j["name_visible"] = name_visible;
    j["pad_visible"] = pad_visible;
    j["name_orientation"] = pin_name_orientation_lut.lookup_reverse(name_orientation);
    j["decoration"] = decoration.serialize();
    return j;
}

UUID SymbolPin::get_uuid() const
{
    return uuid;
}

static const unsigned int app_version = 1;

unsigned int Symbol::get_app_version()
{
    return app_version;
}

Symbol::Symbol(const UUID &uu, const json &j, IPool &pool)
    : uuid(uu), unit(pool.get_unit(j.at("unit").get<std::string>())), name(j.value("name", "")),
      can_expand(j.value("can_expand", false)), version(app_version, j)
{
    check_object_type(j, ObjectType::SYMBOL);
    version.check(ObjectType::SYMBOL, name, uuid);
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
    if (j.count("pins")) {
        const json &o = j["pins"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            pins.emplace(std::make_pair(u, SymbolPin(u, it.value())));
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
    if (j.count("text_placements")) {
        const json &o = j["text_placements"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            std::string view_str(it.key());
            int angle = std::stoi(view_str);
            int mirror = view_str.find("m") != std::string::npos;
            const json &k = it.value();
            for (auto it2 = k.cbegin(); it2 != k.cend(); ++it2) {
                UUID u(it2.key());
                if (texts.count(u)) {
                    Placement placement(it2.value());
                    if (version.get_file() == 0) { // has broken text placement
                        if (placement.mirror)
                            placement.set_angle(-placement.get_angle());
                    }
                    std::tuple<int, bool, UUID> key(angle, mirror, u);
                    text_placements[key] = placement;
                }
            }
        }
    }
}

Symbol::Symbol(const UUID &uu) : uuid(uu), version(app_version)
{
}

Symbol Symbol::new_from_file(const std::string &filename, IPool &pool)
{
    auto j = load_json_from_file(filename);
    return Symbol(UUID(j.at("uuid").get<std::string>()), j, pool);
}

Junction *Symbol::get_junction(const UUID &uu)
{
    return junctions.count(uu) ? &junctions.at(uu) : nullptr;
}

void Symbol::update_refs()
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

Symbol::Symbol(const Symbol &sym)
    : uuid(sym.uuid), unit(sym.unit), name(sym.name), pins(sym.pins), junctions(sym.junctions), lines(sym.lines),
      arcs(sym.arcs), texts(sym.texts), polygons(sym.polygons), can_expand(sym.can_expand),
      text_placements(sym.text_placements), version(sym.version)
{
    update_refs();
}

void Symbol::operator=(Symbol const &sym)
{
    uuid = sym.uuid;
    unit = sym.unit;
    name = sym.name;
    pins = sym.pins;
    junctions = sym.junctions;
    lines = sym.lines;
    arcs = sym.arcs;
    texts = sym.texts;
    polygons = sym.polygons;
    can_expand = sym.can_expand;
    text_placements = sym.text_placements;
    version = sym.version;
    update_refs();
}

void Symbol::expand(PinDisplayMode mode)
{
    std::vector<UUID> keys;
    keys.reserve(pins.size());
    for (const auto &it : pins) {
        keys.push_back(it.first);
    }
    for (const auto &uu : keys) {
        if (unit->pins.count(uu)) {
            SymbolPin &p = pins.at(uu);
            p.pad = "$PAD";
            switch (mode) {
            case PinDisplayMode::PRIMARY:
                p.name = unit->pins.at(uu).primary_name;
                break;
            case PinDisplayMode::ALT:
            case PinDisplayMode::BOTH:
                p.name = "";
                for (auto &[uu_alt, pin_name] : unit->pins.at(uu).names) {
                    p.name += pin_name.name + " ";
                }
                if (mode == PinDisplayMode::BOTH) {
                    p.name += "(" + unit->pins.at(uu).primary_name + ")";
                }
                break;
            }
            p.direction = unit->pins.at(uu).direction;
        }
        else {
            pins.erase(uu);
        }
    }
}

void Symbol::update_junction_connections()
{

    for (auto &[uu, it] : junctions) {
        it.clear();
    }

    JunctionUtil::update(lines);
    JunctionUtil::update(arcs);
}

std::pair<Coordi, Coordi> Symbol::get_bbox(bool all) const
{
    BBoxAccumulator<Coordi::type> acc;
    for (const auto &it : junctions) {
        acc.accumulate(it.second.position);
    }
    for (const auto &it : arcs) {
        acc.accumulate(it.second.get_bbox());
    }
    for (const auto &it : pins) {
        acc.accumulate(it.second.position);
    }
    if (all) {
        for (const auto &it : texts) {
            acc.accumulate(it.second.placement.shift);
        }
    }
    return acc.get_or_0();
}

void Symbol::apply_placement(const Placement &p)
{
    for (auto &it : texts) {
        std::tuple<int, bool, UUID> key(p.get_angle_deg(), p.mirror, it.second.uuid);
        if (text_placements.count(key)) {
            it.second.placement = text_placements.at(key);
        }
    }
}

static void apply_shift(int64_t &x, int64_t x_orig, int64_t ex)
{
    if (x_orig > 0)
        x = x_orig + ex;
    else
        x = x_orig - ex;
}

void Symbol::apply_expand(const Symbol &ref, unsigned int n_expand)
{
    if (ref.uuid != uuid)
        throw std::logic_error("wrong ref symbol");
    if (!can_expand)
        throw std::logic_error("can't expand");

    int64_t ex = n_expand * 1.25_mm;
    int64_t x_left = 0, x_right = 0;
    for (const auto &[uu, it] : ref.junctions) {
        x_left = std::min(it.position.x, x_left);
        x_right = std::max(it.position.x, x_right);
        apply_shift(junctions.at(uu).position.x, it.position.x, ex);
    }
    for (const auto &[uu, it] : ref.pins) {
        if (it.orientation == Orientation::LEFT || it.orientation == Orientation::RIGHT) {
            apply_shift(pins.at(uu).position.x, it.position.x, ex);
        }
    }
    for (const auto &[uu, it] : ref.texts) {
        if (it.placement.shift.x == x_left) {
            texts.at(uu).placement.shift.x = it.placement.shift.x - ex;
        }
        else if (it.placement.shift.x == x_right) {
            texts.at(uu).placement.shift.x = it.placement.shift.x + ex;
        }
    }
}

unsigned int Symbol::get_required_version() const
{
    if (text_placements.size())
        return 1;
    else
        return 0;
}

json Symbol::serialize() const
{
    json j;
    j["type"] = "symbol";
    j["name"] = name;
    j["uuid"] = (std::string)uuid;
    j["unit"] = (std::string)unit->uuid;
    j["can_expand"] = can_expand;
    if (const auto v = get_required_version())
        j["version"] = v;
    j["junctions"] = json::object();
    for (const auto &it : junctions) {
        j["junctions"][(std::string)it.first] = it.second.serialize();
    }
    j["pins"] = json::object();
    for (const auto &it : pins) {
        j["pins"][(std::string)it.first] = it.second.serialize();
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
    j["text_placements"] = json::object();
    for (const auto &it : text_placements) {
        int angle;
        bool mirror;
        UUID uu;
        std::tie(angle, mirror, uu) = it.first;
        std::string k = std::to_string(angle);
        if (mirror) {
            k += "m";
        }
        else {
            k += "n";
        }
        j["text_placements"][k][(std::string)uu] = it.second.serialize();
    }
    return j;
}
} // namespace horizon
