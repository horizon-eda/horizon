#include "kicad_package_parser.hpp"
#include "util/util.hpp"
#include "pool/package.hpp"
#include "pool/decal.hpp"
#include "sexpr/sexpr.h"
#include "util/util.hpp"
#include "board/board_layers.hpp"
#include "pool/ipool.hpp"
#include "logger/logger.hpp"
#include <sstream>
#include <iterator>

namespace horizon {

static void log_logger(const std::string &msg, const std::string &detail)
{
    Logger::get().log_warning(msg, Logger::Domain::IMPORT, detail);
}

KiCadModuleParser::KiCadModuleParser()
{
    log_cb = &log_logger;
}

void KiCadModuleParser::set_log_cb(LogCb cb)
{
    log_cb = cb;
}


KiCadPackageParser::KiCadPackageParser(Package &p, IPool &po) : package(p), pool(po)
{
}

KiCadDecalParser::KiCadDecalParser(Decal &d) : decal(d)
{
}

Junction *KiCadModuleParser::get_or_create_junction(const Coordi &c)
{
    if (junctions.count(c))
        return junctions.at(c);
    else {
        auto &ju = create_junction();
        ju.position = c;
        junctions.emplace(c, &ju);
        return &ju;
    }
}

Coordi KiCadModuleParser::get_coord(const SEXPR::SEXPR *data, size_t offset)
{
    auto c = get_size(data, offset);
    return Coordi(c.x, -c.y);
}

Coordi KiCadModuleParser::get_size(const SEXPR::SEXPR *data, size_t offset)
{
    auto x = data->GetChild(offset)->GetDouble();
    auto y = data->GetChild(offset + 1)->GetDouble();
    return Coordi(x * 1_mm, y * 1_mm);
}

int KiCadModuleParser::get_layer(const std::string &l)
{
    if (l == "F.SilkS")
        return BoardLayers::TOP_SILKSCREEN;
    else if (l == "F.Fab")
        return BoardLayers::TOP_ASSEMBLY;
    else if (l == "F.CrtYd")
        return BoardLayers::TOP_COURTYARD;
    else if (l == "F.Cu")
        return BoardLayers::TOP_COPPER;
    else if (l == "*.Cu")
        return BoardLayers::TOP_COPPER;
    else if (l == "F.Paste")
        return BoardLayers::TOP_PASTE;
    else if (l == "F.Mask")
        return BoardLayers::TOP_MASK;
    else if (l == "*.Mask")
        return BoardLayers::TOP_MASK;
    else {
        throw std::runtime_error("unsupported layer " + l);
        return 10000;
    }
}

static auto get_string_or_symbol(const SEXPR::SEXPR *d)
{
    if (d->IsString())
        return d->GetString();
    else
        return d->GetSymbol();
}

Line *KiCadModuleParser::parse_line(const SEXPR::SEXPR *data)
{
    auto nc = data->GetNumberOfChildren();
    Coordi start;
    Coordi end;
    int layer = 10000;
    uint64_t width = 0;

    for (size_t i_ch = 0; i_ch < nc; i_ch++) {
        auto ch = data->GetChild(i_ch);
        if (ch->IsList()) {
            auto tag = ch->GetChild(0)->GetSymbol();
            if (tag == "start") {
                start = get_coord(ch);
            }
            else if (tag == "end") {
                end = get_coord(ch);
            }
            else if (tag == "layer") {
                const auto l = get_string_or_symbol(ch->GetChild(1));
                try {
                    layer = get_layer(l);
                }
                catch (...) {
                    return nullptr; // ignore
                }
            }
            else if (tag == "width") {
                width = ch->GetChild(1)->GetDouble() * 1_mm;
            }
        }
    }

    auto from = get_or_create_junction(start);
    auto to = get_or_create_junction(end);
    auto &line = create_line();
    line.from = from;
    line.to = to;
    line.width = width;
    line.layer = layer;
    return &line;
}
void KiCadPackageParser::parse_pad(const SEXPR::SEXPR *data)
{
    auto nc = data->GetNumberOfChildren();
    std::string name;
    {
        auto ch = data->GetChild(1);
        if (ch->IsSymbol()) {
            name = ch->GetSymbol();
        }
        else if (ch->IsString()) {
            name = ch->GetString();
        }
        else if (ch->IsInteger()) {
            name = std::to_string(ch->GetInteger());
        }
    }
    auto type = data->GetChild(2)->GetSymbol();
    auto shape = data->GetChild(3)->GetSymbol();

    Coordi pos;
    Coordi size;
    int angle = 0;
    int drill = 0;
    int drill_length = 0;
    std::set<int> layers;
    double roundrect_rratio = 1;

    for (size_t i_ch = 0; i_ch < nc; i_ch++) {
        auto ch = data->GetChild(i_ch);
        if (ch->IsList()) {
            auto tag = ch->GetChild(0)->GetSymbol();
            if (tag == "at") {
                pos = get_coord(ch);
                if (ch->GetNumberOfChildren() >= 4) {
                    angle = ch->GetChild(3)->GetInteger();
                }
            }
            else if (tag == "size") {
                size = get_size(ch);
            }
            else if (tag == "layers") {
                auto n = ch->GetNumberOfChildren();
                for (size_t i_layer = 1; i_layer < n; i_layer++) {
                    layers.insert(get_layer(get_string_or_symbol(ch->GetChild(i_layer))));
                }
            }
            else if (tag == "drill") {
                if (ch->GetChild(1)->IsSymbol() && (ch->GetChild(1)->GetSymbol() == "oval")) {
                    drill = ch->GetChild(2)->GetDouble() * 1_mm;
                    drill_length = ch->GetChild(3)->GetDouble() * 1_mm;
                }
                else {
                    drill = ch->GetChild(1)->GetDouble() * 1_mm;
                }
            }
            else if (tag == "roundrect_rratio") {
                roundrect_rratio = ch->GetChild(1)->GetDouble();
            }
        }
    }

    enum class PadType { INVALID, SMD_RECT, SMD_RECT_ROUND, SMD_OBROUND, SMD_CIRC, TH_CIRC, NPTH_CIRC, TH_OBROUND };
    PadType pad_type = PadType::INVALID;

    std::string padstack_name;
    if (type == "smd" && shape == "rect" && layers.count(BoardLayers::TOP_COPPER)) {
        padstack_name = "smd rectangular";
        pad_type = PadType::SMD_RECT;
    }
    else if (type == "smd" && shape == "oval" && layers.count(BoardLayers::TOP_COPPER)) {
        padstack_name = "smd obround";
        pad_type = PadType::SMD_OBROUND;
    }
    else if (type == "smd" && shape == "roundrect" && layers.count(BoardLayers::TOP_COPPER)) {
        padstack_name = "smd rectangular rounded";
        pad_type = PadType::SMD_RECT_ROUND;
    }
    else if (type == "smd" && shape == "circle" && layers.count(BoardLayers::TOP_COPPER)) {
        padstack_name = "smd circular";
        pad_type = PadType::SMD_CIRC;
    }
    else if (type == "thru_hole" && shape == "circle" && layers.count(BoardLayers::TOP_COPPER)) {
        padstack_name = "th circular";
        pad_type = PadType::TH_CIRC;
    }
    else if (type == "thru_hole" && shape == "oval" && layers.count(BoardLayers::TOP_COPPER)) {
        padstack_name = "th obround";
        pad_type = PadType::TH_OBROUND;
    }
    else if (type == "np_thru_hole" && shape == "circle") {
        padstack_name = "npth circular";
        pad_type = PadType::NPTH_CIRC;
    }
    else if (type == "thru_hole" && shape == "rect" && layers.count(BoardLayers::TOP_COPPER) && name == "1") {
        padstack_name = "th circular";
        pad_type = PadType::TH_CIRC;
    }
    else if (type == "thru_hole" && shape == "roundrect" && layers.count(BoardLayers::TOP_COPPER) && name == "1") {
        padstack_name = "th circular";
        pad_type = PadType::TH_CIRC;
    }
    else {
        log_cb("Pad '" + name + "' is unsupported", "type=" + type + " shape=" + shape);
    }
    const Padstack *padstack = nullptr;
    if (padstack_name.size()) {
        padstack = pool.get_well_known_padstack(padstack_name);
        if (!padstack) {
            log_cb("Well-kown padstack '" + padstack_name + "' not found in pool", "");
        }
    }

    if (padstack) {
        auto uu = UUID::random();
        auto &pad = package.pads
                            .emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                     std::forward_as_tuple(uu, padstack))
                            .first->second;
        pad.name = name;
        pad.placement.shift = pos;
        pad.placement.set_angle_deg(angle);
        if (pad_type == PadType::SMD_RECT) {
            pad.parameter_set[ParameterID::PAD_WIDTH] = size.x;
            pad.parameter_set[ParameterID::PAD_HEIGHT] = size.y;
        }
        else if (pad_type == PadType::SMD_OBROUND) {
            if (size.y > size.x) {
                std::swap(size.x, size.y);
                pad.placement.inc_angle_deg(90);
                if (pad.placement.get_angle_deg() == 180)
                    pad.placement.set_angle(0);
            }
            pad.parameter_set[ParameterID::PAD_WIDTH] = size.x;
            pad.parameter_set[ParameterID::PAD_HEIGHT] = size.y;
        }
        else if (pad_type == PadType::SMD_RECT_ROUND) {
            pad.parameter_set[ParameterID::PAD_WIDTH] = size.x;
            pad.parameter_set[ParameterID::PAD_HEIGHT] = size.y;
            pad.parameter_set[ParameterID::CORNER_RADIUS] = std::min(size.x, size.y) * roundrect_rratio;
        }
        else if (pad_type == PadType::SMD_CIRC) {
            pad.parameter_set[ParameterID::PAD_DIAMETER] = size.x;
        }
        else if (pad_type == PadType::TH_CIRC) {
            pad.parameter_set[ParameterID::PAD_DIAMETER] = size.x;
            pad.parameter_set[ParameterID::HOLE_DIAMETER] = drill;
        }
        else if (pad_type == PadType::NPTH_CIRC) {
            pad.parameter_set[ParameterID::HOLE_DIAMETER] = drill;
        }
        else if (pad_type == PadType::TH_OBROUND) {
            if (size.x > size.y) {
                pad.parameter_set[ParameterID::PAD_WIDTH] = size.x;
                pad.parameter_set[ParameterID::PAD_HEIGHT] = size.y;
                pad.parameter_set[ParameterID::HOLE_DIAMETER] = drill_length;
                pad.parameter_set[ParameterID::HOLE_LENGTH] = drill;
            }
            else {
                pad.parameter_set[ParameterID::PAD_WIDTH] = size.y;
                pad.parameter_set[ParameterID::PAD_HEIGHT] = size.x;
                pad.parameter_set[ParameterID::HOLE_DIAMETER] = drill;
                pad.parameter_set[ParameterID::HOLE_LENGTH] = drill_length;
                pad.placement.inc_angle_deg(90);
            }
        }
    }
}

void KiCadModuleParser::parse_poly(const SEXPR::SEXPR *data)
{
    const SEXPR::SEXPR *pts = nullptr;
    int layer = 10000;

    const auto nc = data->GetNumberOfChildren();
    for (size_t i_ch = 1; i_ch < nc; i_ch++) {
        auto ch = data->GetChild(i_ch);
        const auto &tag = ch->GetChild(0)->GetSymbol();
        if (tag == "pts") {
            pts = ch;
        }
        else if (tag == "layer") {
            layer = get_layer(ch->GetChild(1)->GetSymbol());
        }
    }

    if (!pts) {
        throw std::runtime_error("no points for polygon");
    }
    if (layer == 10000) {
        throw std::runtime_error("no layer for polygon");
    }

    auto &poly = create_polygon();
    poly.layer = layer;

    const auto npts = pts->GetNumberOfChildren();
    for (size_t i = 1; i < npts; i++) {
        auto c = get_coord(pts->GetChild(i), 1);
        poly.vertices.emplace_back(c);
    }
}

KiCadPackageParser::Meta KiCadPackageParser::parse(const SEXPR::SEXPR *data)
{
    KiCadPackageParser::Meta ret;
    if (!data->IsList())
        throw std::runtime_error("data must be list");
    {
        auto ch = data->GetChild(0);
        if (ch->GetSymbol() != "module" && ch->GetSymbol() != "footprint") {
            throw std::runtime_error("not a module or footprint");
        }
    }
    ret.name = get_string_or_symbol(data->GetChild(1));
    auto nc = data->GetNumberOfChildren();
    for (size_t i_ch = 0; i_ch < nc; i_ch++) {
        auto ch = data->GetChild(i_ch);
        if (ch->IsList()) {
            auto type = ch->GetChild(0)->GetSymbol();
            if (type == "fp_line") {
                if (auto line = parse_line(ch)) {
                    if (line->layer != BoardLayers::TOP_ASSEMBLY && line->layer != BoardLayers::TOP_SILKSCREEN) {
                        package.lines.erase(line->uuid);
                    }
                    else if (line->layer == BoardLayers::TOP_ASSEMBLY) {
                        line->width = 0;
                    }
                }
            }
            else if (type == "pad") {
                parse_pad(ch);
            }
            else if (type == "descr") {
                ret.descr = get_string_or_symbol(ch->GetChild(1));
            }
            else if (type == "tags") {
                auto tags_str = get_string_or_symbol(ch->GetChild(1));
                std::transform(tags_str.begin(), tags_str.end(), tags_str.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                std::stringstream ss(tags_str);
                std::istream_iterator<std::string> begin(ss);
                std::istream_iterator<std::string> end;
                std::set<std::string> tags(begin, end);
                ret.tags = tags;
            }
        }
    }
    return ret;
}

Junction &KiCadPackageParser::create_junction()
{
    const auto uu = UUID::random();
    return package.junctions.emplace(uu, uu).first->second;
}

Polygon &KiCadPackageParser::create_polygon()
{
    const auto uu = UUID::random();
    return package.polygons.emplace(uu, uu).first->second;
}

Line &KiCadPackageParser::create_line()
{
    const auto uu = UUID::random();
    return package.lines.emplace(uu, uu).first->second;
}


void KiCadDecalParser::parse(const SEXPR::SEXPR *data)
{
    if (!data->IsList())
        throw std::runtime_error("data must be list");
    {
        auto ch = data->GetChild(0);
        if (ch->GetSymbol() != "module") {
            throw std::runtime_error("not a module");
        }
    }
    auto nc = data->GetNumberOfChildren();
    for (size_t i_ch = 0; i_ch < nc; i_ch++) {
        auto ch = data->GetChild(i_ch);
        if (ch->IsList()) {
            auto type = ch->GetChild(0)->GetSymbol();
            if (type == "fp_line") {
                parse_line(ch);
            }
            else if (type == "fp_poly") {
                parse_poly(ch);
            }
        }
    }
}

Junction &KiCadDecalParser::create_junction()
{
    const auto uu = UUID::random();
    return decal.junctions.emplace(uu, uu).first->second;
}

Polygon &KiCadDecalParser::create_polygon()
{
    const auto uu = UUID::random();
    return decal.polygons.emplace(uu, uu).first->second;
}

Line &KiCadDecalParser::create_line()
{
    const auto uu = UUID::random();
    return decal.lines.emplace(uu, uu).first->second;
}


} // namespace horizon
