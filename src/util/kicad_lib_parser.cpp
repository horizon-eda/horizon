#include "kicad_lib_parser.hpp"
#include "util/util.hpp"
#include <iostream>
#include <iterator>
#include <sstream>
#include "pool/part.hpp"
#include "pool/symbol.hpp"
#include "pool/entity.hpp"
#include "util/str_util.hpp"
#include "sexpr/sexpr.h"
#include "sexpr/sexpr_parser.h"
#include <glibmm/fileutils.h>
#include <numeric>

namespace horizon {

static std::vector<std::string> get_fields(const std::string &line)
{
    std::stringstream ss(line);
    std::istream_iterator<std::string> begin(ss);
    std::istream_iterator<std::string> end;
    return {begin, end};
}

Coordi coordi_from_fields(const std::vector<std::string> &fields, size_t first)
{
    return Coordi(std::stoll(fields.at(first)), std::stoll(fields.at(first + 1)));
}

static const std::map<std::string, Orientation> orientation_map = {
        {"D", Orientation::DOWN},
        {"U", Orientation::UP},
        {"L", Orientation::LEFT},
        {"R", Orientation::RIGHT},
};

static const std::map<std::string, Pin::Direction> direction_map = {
        {"I", Pin::Direction::INPUT},       {"O", Pin::Direction::OUTPUT},        {"B", Pin::Direction::BIDIRECTIONAL},
        {"T", Pin::Direction::INPUT},       {"P", Pin::Direction::PASSIVE},       {"C", Pin::Direction::OPEN_COLLECTOR},
        {"E", Pin::Direction::OUTPUT},      {"N", Pin::Direction::NOT_CONNECTED}, {"U", Pin::Direction::INPUT},
        {"W", Pin::Direction::POWER_INPUT}, {"w", Pin::Direction::POWER_OUTPUT},
};

class DCMInfo {
public:
    std::string description;
    std::string datasheet;
};


std::map<std::string, DCMInfo> parse_dcm(const std::string &filename)
{
    std::map<std::string, DCMInfo> r;
    DCMInfo *cmp = nullptr;
    auto ifs = make_ifstream(filename);
    std::string line;
    while (std::getline(ifs, line)) {
        const auto fields = get_fields(line);
        if (fields.size()) {
            if (fields.at(0) == "$CMP") {
                if (cmp) {
                    throw std::runtime_error("repeated $CMP");
                }
                else {
                    cmp = &r[fields.at(1)];
                }
            }
            else if (fields.at(0) == "$ENDCMP") {
                if (cmp) {
                    cmp = nullptr;
                }
                else {
                    throw std::runtime_error("no cmp to end");
                }
            }
            else if (fields.at(0) == "D" && cmp) {
                cmp->description = line.substr(2);
            }
            else if (fields.at(0) == "F" && cmp) {
                cmp->datasheet = line.substr(2);
            }
        }
    }
    return r;
}

static std::list<KiCadSymbol> parse_kicad_library_old(const std::string &filename)
{
    std::map<std::string, DCMInfo> dcm_info;
    if (endswith(filename, ".lib")) {
        auto dcm_filename = filename.substr(0, filename.size() - 4) + ".dcm";
        if (Glib::file_test(dcm_filename, Glib::FILE_TEST_IS_REGULAR)) {
            dcm_info = parse_dcm(dcm_filename);
        }
    }

    std::list<KiCadSymbol> symbols;
    KiCadSymbol *sym = nullptr;
    bool draw_mode = false;
    bool fplist_mode = false;
    auto ifs = make_ifstream(filename);
    std::string line;
    while (std::getline(ifs, line)) {
        if (fplist_mode) {
            if (line == "$ENDFPLIST") {
                fplist_mode = false;
            }
            else {
                trim(line);
                sym->fplist.push_back(line);
            }
        }
        else {
            const auto fields = get_fields(line);
            if (fields.size()) {
                if (fields.at(0) == "DEF") {
                    if (!sym) {
                        symbols.emplace_back();
                        sym = &symbols.back();
                        sym->name = fields.at(1);
                        sym->prefix = fields.at(2);
                        if (dcm_info.count(sym->name)) {
                            sym->datasheet = dcm_info.at(sym->name).datasheet;
                            sym->description = dcm_info.at(sym->name).description;
                        }
                        sym->parts.resize(std::stoul(fields.at(7)));
                    }
                    else {
                        throw std::runtime_error("repeated DEF");
                    }
                }
                else if (fields.at(0) == "ENDDEF") {
                    if (sym) {
                        sym = nullptr;
                        if (draw_mode)
                            throw std::runtime_error("can't end in draw mode");
                        if (fplist_mode)
                            throw std::runtime_error("can't end in FPLIST mode");
                    }
                    else {
                        throw std::runtime_error("no DEF to end");
                    }
                }
                else if (fields.at(0) == "DRAW") {
                    if (!sym)
                        throw std::runtime_error("no sym to DRAW");
                    if (!draw_mode)
                        draw_mode = true;
                    else
                        throw std::runtime_error("repeated DRAW");
                }
                else if (fields.at(0) == "ENDDRAW") {
                    if (draw_mode)
                        draw_mode = false;
                    else
                        throw std::runtime_error("repeated ENDDRAW");
                }
                else if (fields.at(0) == "F2") {
                    if (sym)
                        sym->footprint = fields.at(1);
                    else
                        throw std::runtime_error("F2: no symbol");
                }
                else if (fields.at(0) == "$FPLIST") {
                    if (sym)
                        fplist_mode = true;
                    else
                        throw std::runtime_error("$FPLIST: no symbol");
                }

                else if (fields.at(0) == "S" && draw_mode) {
                    for (auto &part : sym->get_parts(std::stoi(fields.at(5)))) {
                        part.rects.emplace_back();
                        auto &r = part.rects.back();
                        r.from = coordi_from_fields(fields, 1);
                        r.to = coordi_from_fields(fields, 3);
                    }
                }
                else if (fields.at(0) == "P" && draw_mode) {
                    for (auto &part : sym->get_parts(std::stoi(fields.at(2)))) {
                        part.polys.emplace_back();
                        auto &p = part.polys.back();
                        size_t nv = std::stoul(fields.at(1));
                        p.vertices.reserve(nv);
                        for (size_t i = 0; i < nv; i++) {
                            p.vertices.push_back(coordi_from_fields(fields, 5 + 2 * i));
                        }
                        if (p.vertices.back() == p.vertices.front()) {
                            p.vertices.pop_back();
                            p.is_closed = true;
                        }
                    }
                }
                else if (fields.at(0) == "X" && draw_mode) {
                    for (auto &part : sym->get_parts(std::stoi(fields.at(9)))) {
                        part.pins.emplace_back();
                        auto &p = part.pins.back();
                        p.name = fields.at(1);
                        p.pad = fields.at(2);
                        p.pos = coordi_from_fields(fields, 3);
                        p.length = std::stoul(fields.at(5));
                        p.orientation = orientation_map.at(fields.at(6));
                        p.direction = direction_map.at(fields.at(11));
                    }
                }
            }
        }
    }

    return symbols;
}

static std::string slurp_from_file(const std::string &filename)
{
    auto ifs = make_ifstream(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("not opened");
    }
    std::stringstream text;
    text << ifs.rdbuf();
    return text.str();
}

static const double scale_factor = 100. / 2.54;

static Coordi get_size(const SEXPR::SEXPR *data, size_t offset = 1)
{
    auto x = data->GetChild(offset)->GetDouble();
    auto y = data->GetChild(offset + 1)->GetDouble();
    return Coordi(x * scale_factor, y * scale_factor);
}

static Coordi get_coord(const SEXPR::SEXPR *data, size_t offset = 1)
{
    auto c = get_size(data, offset);
    return Coordi(c.x, c.y);
}


static void parse_pin(KiCadSymbol::SPart::SPin &pin, const SEXPR::SEXPR *kpin)
{
    {
        const auto dir = kpin->GetChild(1)->GetSymbol();
        static const std::map<std::string, Pin::Direction> dmap = {
                {"input", Pin::Direction::INPUT},
                {"output", Pin::Direction::OUTPUT},
                {"bidirectional", Pin::Direction::BIDIRECTIONAL},
                {"tri_state", Pin::Direction::BIDIRECTIONAL},
                {"passive", Pin::Direction::PASSIVE},
                {"free", Pin::Direction::NOT_CONNECTED},
                {"unspecified", Pin::Direction::BIDIRECTIONAL},
                {"power_in", Pin::Direction::POWER_INPUT},
                {"power_out", Pin::Direction::POWER_OUTPUT},
                {"open_collector", Pin::Direction::OPEN_COLLECTOR},
                {"open_emitter", Pin::Direction::OUTPUT},
                {"no_connect", Pin::Direction::NOT_CONNECTED},
        };
        pin.direction = dmap.at(dir);
    }
    for (const auto &child : *kpin->GetChildren()) {
        if (child->IsList() && child->GetChild(0)->IsSymbol()) {
            if (child->GetChild(0)->GetSymbol() == "name") {
                const auto n = child->GetChild(1)->GetString();
                pin.name.reserve(n.size());
                for (const auto c : n) {
                    if (c != '{' && c != '}')
                        pin.name.append(1, c);
                }
            }
            else if (child->GetChild(0)->GetSymbol() == "number") {
                pin.pad = child->GetChild(1)->GetString();
            }
            else if (child->GetChild(0)->GetSymbol() == "at") {
                pin.pos = get_coord(child);
                auto angle = child->GetChild(3)->GetInteger();
                static const std::map<int, Orientation> omap = {
                        {0, Orientation::RIGHT},
                        {90, Orientation::UP},
                        {180, Orientation::LEFT},
                        {270, Orientation::DOWN},
                };
                pin.orientation = omap.at(angle);
            }
            else if (child->GetChild(0)->GetSymbol() == "length") {
                pin.length = child->GetChild(1)->GetDouble() * scale_factor;
            }
        }
    }
}

static void parse_rect(KiCadSymbol::SPart::SRect &rect, const SEXPR::SEXPR *kpin)
{
    for (const auto &child : *kpin->GetChildren()) {
        if (child->IsList() && child->GetChild(0)->IsSymbol()) {
            if (child->GetChild(0)->GetSymbol() == "start") {
                rect.from = get_coord(child);
            }
            else if (child->GetChild(0)->GetSymbol() == "end") {
                rect.to = get_coord(child);
            }
        }
    }
}

static void parse_polyline(KiCadSymbol::SPart::SPolyline &polyline, const SEXPR::SEXPR *kpolyline)
{
    auto pts_list = kpolyline->GetChild(1);
    polyline.pts.reserve(pts_list->GetNumberOfChildren() - 1);
    for (size_t i = 1; i < pts_list->GetNumberOfChildren(); i++) {
        polyline.pts.push_back(get_coord(pts_list->GetChild(i)));
    }
}

static void parse_part(KiCadSymbol::SPart &part, const SEXPR::SEXPR *kpart)
{
    for (const auto &child : *kpart->GetChildren()) {
        if (child->IsList() && child->GetChild(0)->IsSymbol() && child->GetChild(0)->GetSymbol() == "pin") {
            bool hide = false;
            for (const auto ch : *child->GetChildren()) {
                if (ch->IsSymbol() && ch->GetSymbol() == "hide") {
                    hide = true;
                    break;
                }
            }
            if (!hide) {
                part.pins.emplace_back();
                parse_pin(part.pins.back(), child);
            }
        }
        else if (child->IsList() && child->GetChild(0)->IsSymbol() && child->GetChild(0)->GetSymbol() == "rectangle") {
            part.rects.emplace_back();
            parse_rect(part.rects.back(), child);
        }
        else if (child->IsList() && child->GetChild(0)->IsSymbol() && child->GetChild(0)->GetSymbol() == "polyline") {
            part.polylines.emplace_back();
            parse_polyline(part.polylines.back(), child);
        }
    }
}

static void parse_symbol(KiCadSymbol &sym, const SEXPR::SEXPR *ksym)
{
    sym.name = ksym->GetChild(1)->GetString();
    KiCadSymbol::SPart global_part;
    for (const auto &child : *ksym->GetChildren()) {
        if (child->IsList() && child->GetChild(0)->IsSymbol() && child->GetChild(0)->GetSymbol() == "symbol") {
            const auto subsym_name = child->GetChild(1)->GetString();
            const auto subsym_info = subsym_name.substr(sym.name.size() + 1);
            const auto underscore_pos = subsym_info.find('_');
            const unsigned int unit = std::stoi(subsym_info.substr(0, underscore_pos));
            if (sym.parts.size() <= unit)
                sym.parts.resize(unit);
            const auto style = std::stoi(subsym_info.substr(underscore_pos + 1));
            // we only care about style 1 and 0 (global to all styles)
            if (style <= 1) {
                if (unit == 0) {
                    parse_part(global_part, child);
                }
                else {
                    parse_part(sym.parts.at(unit - 1), child);
                }
            }
        }
        if (child->IsList() && child->GetChild(0)->IsSymbol() && child->GetChild(0)->GetSymbol() == "property") {
            const auto key = child->GetChild(1)->GetString();
            const auto value = child->GetChild(2)->GetString();
            if (key == "Footprint")
                sym.footprint = value;
            else if (key == "Datasheet")
                sym.datasheet = value;
            else if (key == "ki_description")
                sym.description = value;
            else if (key == "Reference")
                sym.prefix = value;
        }
    }
    for (auto &part : sym.parts) {
        for (const auto &pin : global_part.pins) {
            part.pins.push_back(pin);
        }
        for (const auto &rect : global_part.rects) {
            part.rects.push_back(rect);
        }
        for (const auto &poly : global_part.polys) {
            part.polys.push_back(poly);
        }
        for (const auto &poly : global_part.polylines) {
            part.polylines.push_back(poly);
        }
    }
}

static std::list<KiCadSymbol> parse_kicad_library_new(const std::string &filename)
{
    SEXPR::PARSER parser;
    std::list<KiCadSymbol> symbols;

    std::unique_ptr<SEXPR::SEXPR> sexpr_data(parser.Parse(slurp_from_file(filename)));
    for (const auto &child : *sexpr_data->GetChildren()) {
        if (child->IsList() && child->GetChild(0)->IsSymbol() && child->GetChild(0)->GetSymbol() == "symbol") {
            if (child->GetChild(2)->IsList() && child->GetChild(2)->GetChild(0)->GetSymbol() == "extends")
                continue;
            symbols.emplace_back();
            parse_symbol(symbols.back(), child);
        }
    }
    return symbols;
}

std::list<KiCadSymbol> parse_kicad_library(const std::string &filename)
{
    if (endswith(filename, ".lib"))
        return parse_kicad_library_old(filename);
    else if (endswith(filename, ".kicad_sym"))
        return parse_kicad_library_new(filename);
    else
        return {};
}


KiCadSymbol::SPartIterProxy KiCadSymbol::get_parts(int idx)
{
    return SPartIterProxy(parts, idx);
}

unsigned int KiCadSymbol::get_n_pins() const
{
    return std::accumulate(parts.begin(), parts.end(), 0,
                           [](const auto c, const auto &x) { return c + x.pins.size(); });
}

static int64_t conv(int64_t x)
{
    return x * (1.25_mm / 50);
}

static Coordi conv(const Coordi &c)
{
    return Coordi(conv(c.x), conv(c.y));
}

static Orientation conv(Orientation o)
{
    switch (o) {
    case Orientation::UP:
        return Orientation::DOWN;
    case Orientation::DOWN:
        return Orientation::UP;
    case Orientation::LEFT:
        return Orientation::RIGHT;
    case Orientation::RIGHT:
        return Orientation::LEFT;
    default:
        return o;
    }
}
KiCadSymbolImporter::KiCadSymbolImporter(const KiCadSymbol &ksym, std::shared_ptr<const class Package> pkg,
                                         bool merge_pins)
    : entity(std::make_shared<Entity>(UUID::random())), part(UUID::random())
{
    char suffix = 0;
    if (ksym.parts.size() > 1)
        suffix = 'A';
    entity->name = ksym.name;
    entity->prefix = ksym.prefix;
    part.attributes[Part::Attribute::MPN] = {false, ksym.name};
    part.attributes[Part::Attribute::DESCRIPTION] = {false, ksym.description};
    part.attributes[Part::Attribute::DATASHEET] = {false, ksym.datasheet};
    part.entity = entity;
    part.package = pkg;
    for (const auto &it : ksym.parts) {
        units.emplace_back(std::make_unique<Unit>(UUID::random()));
        symbols.emplace_back(UUID::random());
        auto unit = units.back();
        auto &sym = symbols.back();
        sym.unit = unit;

        std::string sfx;
        if (suffix) {
            sfx = std::string(1, suffix);
            suffix++;
        }
        unit->name = ksym.name + " " + sfx;
        trim(unit->name);
        sym.name = unit->name;

        auto gate_uu = UUID::random();
        auto &gate = entity->gates.emplace(gate_uu, gate_uu).first->second;
        if (suffix)
            gate.name = sfx;
        else
            gate.name = "Main";
        gate.suffix = sfx;
        gate.unit = unit;

        std::map<std::string, UUID> pin_names;

        for (const auto &it_pin : it.pins) {
            if (!merge_pins || it_pin.name == "~" || (merge_pins && !pin_names.count(it_pin.name))) {
                auto pin_uu = UUID::random();
                pin_names.emplace(it_pin.name, pin_uu);
                auto &pin = unit->pins.emplace(pin_uu, pin_uu).first->second;
                auto &sym_pin = sym.pins.emplace(pin_uu, pin_uu).first->second;
                pin.primary_name = it_pin.name;
                pin.direction = it_pin.direction;

                sym_pin.length = conv(it_pin.length);
                sym_pin.orientation = conv(it_pin.orientation);
                sym_pin.position = conv(it_pin.pos);
                sym_pin.name = it_pin.name;
                sym_pin.pad = it_pin.pad;
                sym_pin.direction = it_pin.direction;

                if (pkg) {
                    auto pad = std::find_if(pkg->pads.begin(), pkg->pads.end(),
                                            [&it_pin](const auto &x) { return x.second.name == it_pin.pad; });
                    if (pad != pkg->pads.end()) {
                        part.pad_map.emplace(std::piecewise_construct, std::forward_as_tuple(pad->second.uuid),
                                             std::forward_as_tuple(&gate, &pin));
                    }
                }
            }
            else if (merge_pins && pin_names.count(it_pin.name) && pkg) {
                auto pad = std::find_if(pkg->pads.begin(), pkg->pads.end(),
                                        [&it_pin](const auto &x) { return x.second.name == it_pin.pad; });
                if (pad != pkg->pads.end()) {
                    part.pad_map.emplace(std::piecewise_construct, std::forward_as_tuple(pad->second.uuid),
                                         std::forward_as_tuple(&gate, &unit->pins.at(pin_names.at(it_pin.name))));
                }
            }
        }
        for (const auto &it_rect : it.rects) {
            std::array<Junction *, 4> junctions;
            for (int i = 0; i < 4; i++) {
                auto uu = UUID::random();
                junctions[i] = &sym.junctions.emplace(uu, uu).first->second;
            }
            auto a = conv(it_rect.from);
            auto b = conv(it_rect.to);
            junctions[0]->position = Coordi(a.x, a.y);
            junctions[1]->position = Coordi(a.x, b.y);
            junctions[2]->position = Coordi(b.x, b.y);
            junctions[3]->position = Coordi(b.x, a.y);
            for (int i = 0; i < 4; i++) {
                auto j = (i + 1) % 4;
                auto uu = UUID::random();
                auto &line = sym.lines.emplace(uu, uu).first->second;
                line.from = junctions.at(i);
                line.to = junctions.at(j);
            }
        }
        for (const auto &it_poly : it.polys) {
            Junction *last = nullptr;
            Junction *first = nullptr;
            for (const auto &vert : it_poly.vertices) {
                auto j_uu = UUID::random();
                auto ju = &sym.junctions.emplace(j_uu, j_uu).first->second;
                if (!first)
                    first = ju;
                ju->position = conv(vert);
                if (last) {
                    auto uu = UUID::random();
                    auto &line = sym.lines.emplace(uu, uu).first->second;
                    line.from = last;
                    line.to = ju;
                }
                last = ju;
            }
            if (it_poly.is_closed) {
                auto uu = UUID::random();
                auto &line = sym.lines.emplace(uu, uu).first->second;
                line.from = last;
                line.to = first;
            }
        }
        for (const auto &it_poly : it.polylines) {
            Junction *last = nullptr;
            for (const auto &pt : it_poly.pts) {
                auto j_uu = UUID::random();
                auto ju = &sym.junctions.emplace(j_uu, j_uu).first->second;
                ju->position = conv(pt);
                if (last) {
                    auto uu = UUID::random();
                    auto &line = sym.lines.emplace(uu, uu).first->second;
                    line.from = last;
                    line.to = ju;
                }

                last = ju;
            }
        }
    }
}

std::shared_ptr<const Entity> KiCadSymbolImporter::get_entity()
{
    return entity;
}
const Part *KiCadSymbolImporter::get_part()
{
    if (part.package)
        return &part;
    else
        return nullptr;
}
const std::list<std::shared_ptr<Unit>> &KiCadSymbolImporter::get_units()
{
    return units;
}
const std::list<Symbol> &KiCadSymbolImporter::get_symbols()
{
    return symbols;
}

} // namespace horizon
