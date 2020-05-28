#include "kicad_package_parser.hpp"
#include "util/util.hpp"
#include "pool/package.hpp"
#include "sexpr/sexpr.h"
#include "util/util.hpp"
#include "board/board_layers.hpp"
#include "pool/pool.hpp"
#include "logger/logger.hpp"

namespace horizon {

KiCadPackageParser::KiCadPackageParser(Package &p, Pool *po) : package(p), pool(po)
{
}

Junction *KiCadPackageParser::get_or_create_junction(const Coordi &c)
{
    if (junctions.count(c))
        return junctions.at(c);
    else {
        auto uu = UUID::random();
        auto &ju = package.junctions.emplace(uu, uu).first->second;
        ju.position = c;
        junctions.emplace(c, &ju);
        return &ju;
    }
}

Coordi KiCadPackageParser::get_coord(const SEXPR::SEXPR *data, size_t offset)
{
    auto c = get_size(data, offset);
    return Coordi(c.x, -c.y);
}

Coordi KiCadPackageParser::get_size(const SEXPR::SEXPR *data, size_t offset)
{
    auto x = data->GetChild(offset)->GetDouble();
    auto y = data->GetChild(offset + 1)->GetDouble();
    return Coordi(x * 1_mm, y * 1_mm);
}

int KiCadPackageParser::get_layer(const std::string &l)
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

void KiCadPackageParser::parse_line(const SEXPR::SEXPR *data)
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
                auto l = ch->GetChild(1)->GetSymbol();
                try {
                    layer = get_layer(l);
                }
                catch (...) {
                    return; // ignore
                }
            }
            else if (tag == "width") {
                width = ch->GetChild(1)->GetDouble() * 1_mm;
            }
        }
    }

    if (layer == BoardLayers::TOP_SILKSCREEN || layer == BoardLayers::TOP_ASSEMBLY) {
        auto from = get_or_create_junction(start);
        auto to = get_or_create_junction(end);
        auto uu = UUID::random();
        auto &line = package.lines.emplace(uu, uu).first->second;
        line.from = from;
        line.to = to;
        line.width = width;
        line.layer = layer;
        if (layer == BoardLayers::TOP_ASSEMBLY)
            line.width = 0;
    }
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
                    layers.insert(get_layer(ch->GetChild(i_layer)->GetSymbol()));
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
        Logger::get().log_warning("Pad '" + name + "' is unsupported", Logger::Domain::TOOL,
                                  "type=" + type + " shape=" + shape);
    }
    const Padstack *padstack = nullptr;
    if (padstack_name.size()) {
        padstack = pool->get_well_known_padstack(padstack_name);
        if (!padstack) {
            Logger::get().log_warning("Well-kown padstack '" + padstack_name + "' not found in pool",
                                      Logger::Domain::TOOL);
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

void KiCadPackageParser::parse(const SEXPR::SEXPR *data)
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
            if (type == "pad") {
                parse_pad(ch);
            }
        }
    }
}

} // namespace horizon
