#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "board/track.hpp"
#include "canvas.hpp"
#include "common/hole.hpp"
#include "common/polygon.hpp"
#include "common/text.hpp"
#include "core/buffer.hpp"
#include "layer_display.hpp"
#include "package/pad.hpp"
#include "poly2tri/poly2tri.h"
#include "polypartition/polypartition.h"
#include "pool/package.hpp"
#include "pool/padstack.hpp"
#include "pool/symbol.hpp"
#include "schematic/bus_label.hpp"
#include "schematic/bus_ripper.hpp"
#include "schematic/line_net.hpp"
#include "schematic/net_label.hpp"
#include "schematic/power_symbol.hpp"
#include "schematic/sheet.hpp"
#include "frame/frame.hpp"
#include "selectables.hpp"
#include "selection_filter.hpp"
#include "target.hpp"
#include "triangle.hpp"
#include "util/placement.hpp"
#include "util/util.hpp"
#include "iairwire_filter.hpp"
#include "board/board_panel.hpp"
#include "common/picture.hpp"
#include <algorithm>
#include <ctime>
#include <sstream>

namespace horizon {

void Canvas::render(const Junction &junc, bool interactive, ObjectType mode)
{
    ColorP c = ColorP::JUNCTION;
    if (junc.net) {
        if (junc.net->diffpair)
            c = ColorP::DIFFPAIR;
        else
            c = ColorP::NET;
    }
    if (junc.bus) {
        c = ColorP::BUS;
    }
    if (junc.warning) {
        draw_error(junc.position, 2e5, "");
    }
    bool draw = true;

    if (mode == ObjectType::BOARD)
        draw = false;

    auto layer = layer_display.count(junc.layer) ? junc.layer : 10000;

    object_refs_current.emplace_back(ObjectType::JUNCTION, junc.uuid);
    if (draw) {
        if (junc.connection_count == 2) {
            if (show_all_junctions_in_schematic)
                draw_plus(junc.position, 250000, c);
        }
        else if (junc.connection_count >= 3 && mode == ObjectType::SCHEMATIC) {
            draw_line(junc.position, junc.position + Coordi(0, 1000), c, 0, true, 0.75_mm);
            img_line(junc.position, junc.position + Coordi(0, 1000), 0.75_mm, 0, true);
        }
        else if (junc.has_via && mode == ObjectType::SCHEMATIC) {
            // nop
        }
        else {
            draw_cross(junc.position, 0.25_mm, c);
        }
    }
    object_refs_current.pop_back();

    if (interactive) {
        selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, layer);
        targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position), 0, layer);
    }
}

void Canvas::render(const PowerSymbol &sym)
{
    auto c = ColorP::FROM_LAYER;
    transform.shift = sym.junction->position;
    object_refs_current.emplace_back(ObjectType::POWER_SYMBOL, sym.uuid);

    auto style = sym.net->power_symbol_style;
    img_auto_line = img_mode;
    switch (style) {
    case Net::PowerSymbolStyle::GND:
    case Net::PowerSymbolStyle::EARTH: {
        transform_save();
        transform.set_angle(orientation_to_angle(sym.orientation) - 49152);
        draw_line({0, 0}, {0, -1.25_mm}, c, 0);
        if (style == Net::PowerSymbolStyle::GND) {
            draw_line({-1.25_mm, -1.25_mm}, {1.25_mm, -1.25_mm}, c, 0);
            draw_line({-1.25_mm, -1.25_mm}, {0, -2.5_mm}, c, 0);
            draw_line({1.25_mm, -1.25_mm}, {0, -2.5_mm}, c, 0);
        }
        else {
            for (float i = 0.0_mm; i < 1.25_mm; i += 1.50_mm / 3) {
                draw_line({-1.25_mm + i, -1.25_mm - i}, {1.25_mm - i, -1.25_mm - i}, c, 0);
            }
        }
        selectables.append(sym.uuid, ObjectType::POWER_SYMBOL, {0, 0}, {-1.25_mm, -2.5_mm}, {1.25_mm, 0_mm});
        transform_restore();

        if (sym.orientation != Orientation::DOWN) {
            draw_error({0, 0}, 2e5, "Unsupported orientation", true);
        }

        transform.reset();

        int text_angle = 0;
        Coordi text_offset(1.25_mm, -1.875_mm);
        if (sym.mirror) {
            text_offset.x *= -1;
            text_angle = 32768;
        }

        if (sym.junction->net->power_symbol_name_visible)
            draw_text0(sym.junction->position + text_offset, 1.5_mm, sym.junction->net->name, text_angle, false,
                       TextOrigin::CENTER, c, 0);
    } break;

    case Net::PowerSymbolStyle::DOT:
    case Net::PowerSymbolStyle::ANTENNA: {
        transform_save();
        transform.set_angle(orientation_to_angle(sym.orientation) - 16384);
        if (style == Net::PowerSymbolStyle::ANTENNA) {
            draw_line({0, 0}, {0, 2.5_mm}, c, 0);
            draw_line({-1_mm, 1_mm}, {0, 2.5_mm}, c, 0);
            draw_line({1_mm, 1_mm}, {0, 2.5_mm}, c, 0);
            selectables.append(sym.uuid, ObjectType::POWER_SYMBOL, {0, 0}, {-1_mm, 2.5_mm}, {1_mm, 0_mm});
        }
        else {
            draw_line({0, 0}, {0, 1_mm}, c, 0);
            draw_arc({0, 1.75_mm}, 0.75_mm, 0, 2 * M_PI, ColorP::FROM_LAYER, 0, true, 0);
            selectables.append(sym.uuid, ObjectType::POWER_SYMBOL, {0, 0}, {-.75_mm, 2.5_mm}, {.75_mm, 0_mm});
        }
        transform_restore();

        transform.reset();

        Coordi text_offset;
        bool mirror = false;
        const int64_t center_offset = 1.25_mm;
        const int64_t radius = 1.875_mm;
        switch (sym.orientation) {
        case Orientation::UP:
            text_offset = {center_offset, radius};
            mirror = not sym.mirror;
            break;
        case Orientation::RIGHT:
            text_offset = {center_offset + radius, 0.0_mm};
            break;
        case Orientation::DOWN:
            text_offset = {center_offset, -radius};
            mirror = sym.mirror;
            break;
        case Orientation::LEFT:
            text_offset = {center_offset + radius, 0.0_mm};
            mirror = true;
            break;
        }

        int text_angle = 0;
        if (mirror) {
            text_offset.x *= -1;
            text_angle = 32768;
        }

        if (sym.junction->net->power_symbol_name_visible)
            draw_text0(sym.junction->position + text_offset, 1.5_mm, sym.junction->net->name, text_angle, false,
                       TextOrigin::CENTER, c, 0);
    }
    }

    transform.reset();
    img_auto_line = false;

    object_refs_current.pop_back();
}

ObjectRef Canvas::add_line(const std::deque<Coordi> &pts, int64_t width, ColorP color, int layer)
{
    auto uu = UUID::random();
    ObjectRef sr(ObjectType::LINE, uu);
    object_refs_current.push_back(sr);
    triangle_type_current = Triangle::Type::TRACK_PREVIEW;
    if (pts.size() >= 2) {
        for (size_t i = 1; i < pts.size(); i++) {
            auto pt1 = pts.at(i - 1);
            auto pt2 = pts.at(i);
            draw_line(pt1, pt2, color, layer, false, width);
        }
    }
    else {
        object_refs[sr];
    }
    triangle_type_current = Triangle::Type::NONE;
    object_refs_current.pop_back();
    request_push();
    return sr;
}

void Canvas::render(const Line &line, bool interactive, ColorP co)
{
    img_line(line.from->position, line.to->position, line.width, line.layer);
    if (img_mode)
        return;
    triangle_type_current = Triangle::Type::GRAPHICS;
    draw_line(line.from->position, line.to->position, co, line.layer, true, line.width);
    triangle_type_current = Triangle::Type::NONE;
    if (interactive) {
        selectables.append_line(line.uuid, ObjectType::LINE, line.from->position, line.to->position, line.width, 0,
                                line.layer);
    }
}

void Canvas::render(const LineNet &line)
{
    uint64_t width = 0;
    ColorP c = ColorP::NET;
    if (line.net == nullptr) {
        c = ColorP::ERROR;
    }
    else {
        if (line.net->diffpair) {
            c = ColorP::DIFFPAIR;
            width = 0.2_mm;
        }
    }
    if (line.bus) {
        c = ColorP::BUS;
        width = 0.2_mm;
    }
    img_line(line.from.get_position(), line.to.get_position(), width, 0);
    if (img_mode)
        return;
    object_refs_current.emplace_back(ObjectType::LINE_NET, line.uuid);
    draw_line(line.from.get_position(), line.to.get_position(), c, 0, true, width);
    object_refs_current.pop_back();
    selectables.append_line(line.uuid, ObjectType::LINE_NET, line.from.get_position(), line.to.get_position(), width);
}

void Canvas::render(const Track &track, bool interactive)
{
    auto c = ColorP::FROM_LAYER;
    if (track.net == nullptr) {
        c = ColorP::BUS;
    }
    auto width = track.width;
    img_net(track.net);
    img_patch_type(PatchType::TRACK);
    img_line(track.from.get_position(), track.to.get_position(), width, track.layer);
    img_patch_type(PatchType::OTHER);
    img_net(nullptr);
    if (img_mode)
        return;
    auto layer = track.layer;

    auto center = (track.from.get_position() + track.to.get_position()) / 2;
    if (interactive)
        object_refs_current.emplace_back(ObjectType::TRACK, track.uuid);
    draw_line(track.from.get_position(), track.to.get_position(), c, layer, true, width);
    if (track.locked) {
        auto ol = get_overlay_layer(layer);
        draw_lock(center, 0.7 * track.width, ColorP::TEXT_OVERLAY, ol, true);
    }
    if (interactive && show_text_in_tracks && width > 0 && track.net && track.net->name.size() && track.from.is_junc()
        && track.to.is_junc() && (!show_text_in_vias || (!track.from.junc->has_via && !track.to.junc->has_via))) {
        auto overlay_layer = get_overlay_layer(track.layer, true);
        set_lod_size(width);
        auto vec = (track.from.get_position() - track.to.get_position());
        auto length = sqrt(vec.mag_sq());
        Placement p(center);
        p.set_angle_rad(atan2(vec.y, vec.x));
        if (get_flip_view()) {
            p.shift.x *= -1;
            p.invert_angle();
        }
        draw_bitmap_text_box(p, length, width, track.net->name, ColorP::TEXT_OVERLAY, overlay_layer, TextBoxMode::FULL);
        set_lod_size(-1);
    }
    if (interactive)
        object_refs_current.pop_back();
    if (interactive)
        selectables.append_line(track.uuid, ObjectType::TRACK, track.from.get_position(), track.to.get_position(),
                                track.width, 0, track.layer);
}

void Canvas::render(const Airwire &airwire)
{
    draw_line(airwire.from.get_position(), airwire.to.get_position(), ColorP::AIRWIRE, 10000, true, 0);
}

void Canvas::render(const ConnectionLine &line)
{
    if (img_mode)
        return;
    draw_line(line.from.get_position(), line.to.get_position(), ColorP::CONNECTION_LINE);
    selectables.append_line(line.uuid, ObjectType::CONNECTION_LINE, line.from.get_position(), line.to.get_position(),
                            0);
}

void Canvas::render(const SymbolPin &pin, bool interactive, ColorP co)
{
    Coordi p0 = transform.transform(pin.position);
    Coordi p1 = p0;

    Coordi p_name = p0;
    Coordi p_pad = p0;
    Coordi p_nc = p0;

    Orientation pin_orientation = pin.get_orientation_for_placement(transform);

    Orientation name_orientation = Orientation::LEFT;
    Orientation pad_orientation = Orientation::LEFT;
    Orientation nc_orientation = Orientation::LEFT;
    int64_t text_shift = 0.5_mm;
    int64_t text_shift_name = text_shift;
    int64_t schmitt_shift = 1.125_mm;
    int64_t driver_shift = .75_mm;
    int64_t nc_shift = 0.25_mm;
    int64_t length = pin.length;
    auto dot_size = .75_mm;
    if (pin.decoration.dot) {
        length -= dot_size;
    }
    if (pin.decoration.clock) {
        text_shift_name += .75_mm;
        schmitt_shift += .75_mm;
        driver_shift += .75_mm;
    }
    if (pin.decoration.schmitt) {
        text_shift_name += 2_mm;
        driver_shift += 2_mm;
    }
    if (pin.decoration.driver != SymbolPin::Decoration::Driver::DEFAULT) {
        text_shift_name += 1_mm;
    }

    Coordi v_deco;
    switch (pin_orientation) {
    case Orientation::LEFT:
        p1.x += length;
        p_name.x += pin.length + text_shift_name;
        p_pad.x += pin.length - text_shift;
        p_pad.y += text_shift;
        v_deco.x = 1;
        p_nc.x -= nc_shift;
        nc_orientation = Orientation::LEFT;
        name_orientation = Orientation::RIGHT;
        pad_orientation = Orientation::LEFT;
        break;

    case Orientation::RIGHT:
        p1.x -= length;
        p_name.x -= pin.length + text_shift_name;
        p_pad.x -= pin.length - text_shift;
        p_pad.y += text_shift;
        v_deco.x = -1;
        p_nc.x += nc_shift;
        nc_orientation = Orientation::RIGHT;
        name_orientation = Orientation::LEFT;
        pad_orientation = Orientation::RIGHT;
        break;

    case Orientation::UP:
        p1.y -= length;
        p_name.y -= pin.length + text_shift_name;
        p_pad.y -= pin.length - text_shift;
        p_pad.x -= text_shift;
        v_deco.y = -1;
        p_nc.y += nc_shift;
        nc_orientation = Orientation::UP;
        name_orientation = Orientation::DOWN;
        pad_orientation = Orientation::UP;
        break;

    case Orientation::DOWN:
        p1.y += length;
        p_name.y += pin.length + text_shift_name;
        p_pad.y += pin.length - text_shift;
        p_pad.x -= text_shift;
        v_deco.y = 1;
        p_nc.y -= nc_shift;
        nc_orientation = Orientation::DOWN;
        name_orientation = Orientation::UP;
        pad_orientation = Orientation::DOWN;
        break;
    }
    ColorP c_main = co;
    ColorP c_name = ColorP::PIN;
    ColorP c_pad = ColorP::PIN;
    ColorP c_pin = ColorP::PIN;

    if (!pin.name_visible) {
        c_name = ColorP::PIN_HIDDEN;
    }
    if (!pin.pad_visible) {
        c_pad = ColorP::PIN_HIDDEN;
    }
    if (co != ColorP::FROM_LAYER) {
        c_name = co;
        c_pad = co;
        c_pin = co;
    }
    img_auto_line = img_mode;
    if (interactive || pin.name_visible) {
        bool draw_in_line = pin.name_orientation == SymbolPin::NameOrientation::IN_LINE
                            || (pin.name_orientation == SymbolPin::NameOrientation::HORIZONTAL
                                && (pin_orientation == Orientation::LEFT || pin_orientation == Orientation::RIGHT));

        if (draw_in_line) {
            draw_text0(p_name, 1.5_mm, pin.name, orientation_to_angle(name_orientation), false, TextOrigin::CENTER,
                       c_name, 0);
        }
        else { // draw perp
            Placement tr;
            tr.set_angle(orientation_to_angle(pin_orientation));
            auto shift = tr.transform(Coordi(-1_mm, 0));
            draw_text0(p_name + shift, 1.5_mm, pin.name, orientation_to_angle(name_orientation) + 16384, false,
                       TextOrigin::CENTER, c_name, 0, 0, true, TextData::Font::SIMPLEX, true);
        }
    }
    std::pair<Coordf, Coordf> pad_extents;
    if (interactive || pin.pad_visible) {
        pad_extents = draw_text0(p_pad, 0.75_mm, pin.pad, orientation_to_angle(pad_orientation), false,
                                 TextOrigin::BASELINE, c_pad, 0);
    }

    transform_save();
    transform.accumulate(pin.position);
    transform.set_angle(0);
    transform.mirror = false;
    switch (pin_orientation) {
    case Orientation::RIGHT:
        break;
    case Orientation::LEFT:
        transform.mirror ^= true;
        break;
    case Orientation::UP:
        transform.inc_angle_deg(90);
        break;
    case Orientation::DOWN:
        transform.inc_angle_deg(-90);
        transform.mirror = true;
        break;
    }

    if (pin.decoration.dot) {
        draw_arc(Coordf(-((int64_t)pin.length) + dot_size / 2, 0), dot_size / 2, 0, 2 * M_PI, c_main, 0, true, 0);
    }
    if (pin.decoration.clock) {
        draw_line(Coordf(-(int64_t)pin.length, .375_mm), Coordf(-(int64_t)pin.length - .75_mm, 0), c_main, 0, true, 0);
        draw_line(Coordf(-(int64_t)pin.length, -.375_mm), Coordf(-(int64_t)pin.length - .75_mm, 0), c_main, 0, true, 0);
    }
    {
        auto dl = [this, c_pin](float ax, float ay, float bx, float by) {
            draw_line(Coordf(ax * 1_mm, ay * 1_mm), Coordf(bx * 1_mm, by * 1_mm), c_pin, 0, true, 0);
        };
        switch (pin.direction) {
        case Pin::Direction::OUTPUT:
            dl(0, -.6, -1, -.2);
            dl(0, -.6, -1, -1);
            break;
        case Pin::Direction::INPUT:
            dl(-1, -.6, 0, -.2);
            dl(-1, -.6, 0, -1);
            break;
        case Pin::Direction::POWER_INPUT:
            dl(-1, -.6, 0, -.2);
            dl(-1, -.6, 0, -1);
            dl(-1.4, -.6, -.4, -.2);
            dl(-1.4, -.6, -.4, -1);
            break;
        case Pin::Direction::POWER_OUTPUT:
            dl(0, -.6, -1, -.2);
            dl(0, -.6, -1, -1);
            dl(-.4, -.6, -1.4, -.2);
            dl(-.4, -.6, -1.4, -1);
            break;
        case Pin::Direction::BIDIRECTIONAL:
            dl(0, -.6, -1, -.2);
            dl(0, -.6, -1, -1);
            dl(-2, -.6, -1, -.2);
            dl(-2, -.6, -1, -1);
            break;
        case Pin::Direction::NOT_CONNECTED:
            dl(-.4, -1, -1, -.2);
            dl(-.4, -.2, -1, -1);
            break;
        default:;
        }
    }
    transform_restore();

    if (pin.decoration.schmitt) {
        auto dl = [this, c_pin](float ax, float ay, float bx, float by) {
            auto sc = .025_mm;
            draw_line(Coordf(ax * sc, ay * sc), Coordf(bx * sc, by * sc), c_pin, 0, true, 0);
        };
        transform_save();
        transform.accumulate(pin.position);
        transform.set_angle(0);
        transform.mirror = false;
        transform.accumulate(v_deco * (pin.length + schmitt_shift));
        dl(-34, -20, -2, -20);
        dl(34, 20, 2, 20);
        dl(-20, -20, 2, 20);
        dl(-2, -20, 20, 20);

        transform_restore();
    }
    if (pin.decoration.driver != SymbolPin::Decoration::Driver::DEFAULT) {
        auto dl = [this, c_pin](float ax, float ay, float bx, float by) {
            auto sc = .5_mm;
            draw_line(Coordf(ax * sc, ay * sc), Coordf(bx * sc, by * sc), c_pin, 0, true, 0);
        };
        transform_save();
        transform.accumulate(pin.position);
        transform.set_angle(0);
        transform.mirror = false;
        transform.accumulate(v_deco * (pin.length + driver_shift));

        if (pin.decoration.driver != SymbolPin::Decoration::Driver::TRISTATE) {
            dl(0, -1, 1, 0);
            dl(1, 0, 0, 1);
            dl(-1, 0, 0, 1);
            dl(-1, 0, 0, -1);
        }

        switch (pin.decoration.driver) {
        case SymbolPin::Decoration::Driver::OPEN_COLLECTOR_PULLUP:
        case SymbolPin::Decoration::Driver::OPEN_EMITTER_PULLDOWN:
            dl(-1, 0, 1, 0);
            break;
        default:;
        }
        switch (pin.decoration.driver) {
        case SymbolPin::Decoration::Driver::OPEN_COLLECTOR:
        case SymbolPin::Decoration::Driver::OPEN_COLLECTOR_PULLUP:
            dl(-1, -1, 1, -1);
            break;
        case SymbolPin::Decoration::Driver::OPEN_EMITTER:
        case SymbolPin::Decoration::Driver::OPEN_EMITTER_PULLDOWN:
            dl(-1, 1, 1, 1);
            break;
        case SymbolPin::Decoration::Driver::TRISTATE:
            dl(1, 1, -1, 1);
            dl(1, 1, 0, -1);
            dl(-1, 1, 0, -1);
            break;
        default:;
        }

        transform_restore();
    }

    switch (pin.connector_style) {
    case SymbolPin::ConnectorStyle::BOX:
        draw_box(p0, 0.25_mm, c_main, 0, false);
        break;

    case SymbolPin::ConnectorStyle::NC:
        draw_cross(p0, 0.25_mm, c_main, 0, false);
        draw_text0(p_nc, 1.5_mm, "NC", orientation_to_angle(nc_orientation), false, TextOrigin::CENTER, c_pin, 0);
        break;

    case SymbolPin::ConnectorStyle::NONE:
        break;
    }
    if (pin.connected_net_lines.size() > 1) {
        draw_line(p0, p0 + Coordi(0, 10), c_main, 0, false, 0.75_mm);
    }
    draw_line(p0, p1, c_main, 0, false);
    if (interactive)
        selectables.append(pin.uuid, ObjectType::SYMBOL_PIN, p0, Coordf::min(pad_extents.first, Coordf::min(p0, p1)),
                           Coordf::max(pad_extents.second, Coordf::max(p0, p1)));
    img_auto_line = false;
}

static int64_t sq(int64_t x)
{
    return x * x;
}

void Canvas::render(const Arc &arc, bool interactive, ColorP co)
{
    Coordf a(arc.from->position);   // ,b,c;
    Coordf b(arc.to->position);     // ,b,c;
    Coordf c(arc.center->position); // ,b,c;
    float radius0 = sqrt(sq(c.x - a.x) + sq(c.y - a.y));
    float radius1 = sqrt(sq(c.x - b.x) + sq(c.y - b.y));
    float a0 = atan2f(a.y - c.y, a.x - c.x);
    float a1 = atan2f(b.y - c.y, b.x - c.x);
    auto bb = draw_arc2(c, radius0, a0, radius1, a1, co, arc.layer, true, arc.width);
    Coordf t(radius0, radius0);
    if (interactive)
        selectables.append(arc.uuid, ObjectType::ARC, c, bb.first, bb.second, 0, arc.layer);
}

void Canvas::render(const SchematicSymbol &sym)
{
    transform = sym.placement;
    object_refs_current.emplace_back(ObjectType::SCHEMATIC_SYMBOL, sym.uuid);
    render(sym.symbol, true, sym.smashed, ColorP::FROM_LAYER);
    object_refs_current.pop_back();
    for (const auto &it : sym.symbol.pins) {
        targets.emplace_back(UUIDPath<2>(sym.uuid, it.second.uuid), ObjectType::SYMBOL_PIN,
                             transform.transform(it.second.position));
    }
    auto bb = sym.symbol.get_bbox();
    selectables.append(sym.uuid, ObjectType::SCHEMATIC_SYMBOL, {0, 0}, bb.first, bb.second);
    transform.reset();

    for (const auto &it : sym.texts) {
        render(*it, true, ColorP::FROM_LAYER);
    }

    if (sym.component->nopopulate) {
        transform = sym.placement;
        img_auto_line = img_mode;
        draw_line(bb.first - Coordi(0.2_mm, 0.2_mm), bb.second + Coordi(0.2_mm, 0.2_mm), ColorP::NOPOPULATE_X, 0, true,
                  0.2_mm);
        draw_line(Coordi(bb.first.x, bb.second.y) + Coordi(-0.2_mm, 0.2_mm),
                  Coordi(bb.second.x, bb.first.y) + Coordi(0.2_mm, -0.2_mm), ColorP::NOPOPULATE_X, 0, true, 0.2_mm);
        img_auto_line = false;
        transform.reset();
    }
}

void Canvas::render(const Text &text, bool interactive, ColorP co)
{
    bool rev = layer_provider->get_layers().at(text.layer).reverse;
    transform_save();
    transform.accumulate(text.placement);
    auto angle = transform.get_angle();
    if (transform.mirror ^ rev) {
        angle = 32768 - angle;
    }

    img_patch_type(PatchType::TEXT);
    triangle_type_current = Triangle::Type::TEXT;
    auto extents = draw_text0(transform.shift, text.size, text.overridden ? text.text_override : text.text, angle, rev,
                              text.origin, co, text.layer, text.width, true, text.font, false, transform.mirror);
    triangle_type_current = Triangle::Type::NONE;
    // img_text_extents(text, extents);
    img_patch_type(PatchType::OTHER);
    transform_restore();
    if (interactive) {
        selectables.append(text.uuid, ObjectType::TEXT, text.placement.shift, extents.first, extents.second, 0,
                           text.layer);
        targets.emplace_back(text.uuid, ObjectType::TEXT, transform.transform(text.placement.shift), 0, text.layer);
    }
}

template <typename T> static std::string join(const T &v, const std::string &delim)
{
    std::ostringstream s;
    for (const auto &i : v) {
        if (i != *v.begin()) {
            s << delim;
        }
        s << i;
    }
    return s.str();
}

void Canvas::render(const NetLabel &label)
{
    std::string txt = "<no net>";
    auto c = ColorP::NET;
    if (label.junction->net) {
        txt = label.junction->net->name;
        if (label.junction->net->diffpair)
            c = ColorP::DIFFPAIR;
    }
    if (txt == "") {
        txt = "? plz fix";
    }
    if (label.on_sheets.size() > 0 && label.offsheet_refs) {
        txt += " [" + join(label.on_sheets, ",") + "]";
    }

    object_refs_current.emplace_back(ObjectType::NET_LABEL, label.uuid);
    if (label.style == NetLabel::Style::FLAG) {

        std::pair<Coordf, Coordf> extents;
        Coordi shift;
        std::tie(extents.first, extents.second, shift) =
                draw_flag(label.junction->position, txt, label.size, label.orientation, c);
        selectables.append(label.uuid, ObjectType::NET_LABEL, label.junction->position + shift, extents.first,
                           extents.second);
    }
    else {
        auto extents = draw_text0(label.junction->position, label.size, txt, orientation_to_angle(label.orientation),
                                  false, TextOrigin::BASELINE, c);
        selectables.append(label.uuid, ObjectType::NET_LABEL, label.junction->position + Coordi(0, 1000000),
                           extents.first, extents.second);
    }
    object_refs_current.pop_back();
}
void Canvas::render(const BusLabel &label)
{
    std::string txt = "<no bus>";
    if (label.junction->bus) {
        txt = "B:" + label.junction->bus->name;
    }
    if (label.on_sheets.size() > 0 && label.offsheet_refs) {
        txt += " [" + join(label.on_sheets, ",") + "]";
    }

    std::pair<Coordf, Coordf> extents;
    Coordi shift;
    std::tie(extents.first, extents.second, shift) =
            draw_flag(label.junction->position, txt, label.size, label.orientation, ColorP::BUS);
    selectables.append(label.uuid, ObjectType::BUS_LABEL, label.junction->position + shift, extents.first,
                       extents.second);
}

void Canvas::render(const BusRipper &ripper)
{
    auto c = ColorP::BUS;
    auto connector_pos = ripper.get_connector_pos();
    img_auto_line = img_mode;
    draw_line(ripper.junction->position, connector_pos, c);
    if (ripper.connection_count < 1) {
        draw_box(connector_pos, 0.25_mm, c);
    }
    img_auto_line = false;
    int angle = 0;
    switch (ripper.orientation) {
    case Orientation::LEFT:
    case Orientation::DOWN:
        angle = 32768;
        break;

    default:
        angle = 0;
    }
    auto extents = draw_text0(connector_pos + Coordi(0, 0.5_mm), 1.5_mm, ripper.bus_member->name, angle, false,
                              TextOrigin::BASELINE, c);
    targets.emplace_back(ripper.uuid, ObjectType::BUS_RIPPER, connector_pos);
    selectables.append(ripper.uuid, ObjectType::BUS_RIPPER, connector_pos, extents.first, extents.second);
}

void Canvas::render(const Warning &warn)
{
    draw_error(warn.position, 2e5, warn.text);
}

static const Coordf coordf_from_pt(const TPPLPoint &p)
{
    return Coordf(p.x, p.y);
}

void Canvas::render(const Polygon &ipoly, bool interactive, ColorP co)
{
    img_polygon(ipoly);
    if (img_mode)
        return;
    Polygon poly = ipoly.remove_arcs(64);
    if (poly.vertices.size() == 0)
        return;

    if (!layer_display.count(poly.layer))
        return;
    if (auto plane = dynamic_cast<Plane *>(poly.usage.ptr)) {
        triangle_type_current = Triangle::Type::PLANE;
        auto tris = fragment_cache.get_triangles(*plane);
        for (const auto &tri : tris) {
            add_triangle(poly.layer, transform.transform(tri[0]), transform.transform(tri[1]),
                         transform.transform(tri[2]), co);
        }
        for (const auto &frag : plane->fragments) {
            ColorP co_orphan = co;
            if (frag.orphan == true)
                co_orphan = ColorP::FRAG_ORPHAN;

            for (const auto &path : frag.paths) {
                for (size_t i = 0; i < path.size(); i++) {
                    auto &c0 = path[i];
                    auto &c1 = path[(i + 1) % path.size()];
                    draw_line(Coordf(c0.X, c0.Y), Coordf(c1.X, c1.Y), co_orphan, poly.layer);
                }
            }
        }
        if (plane->fragments.size() == 0) { // empty, draw poly outline
            for (size_t i = 0; i < poly.vertices.size(); i++) {
                draw_line(poly.vertices[i].position, poly.vertices[(i + 1) % poly.vertices.size()].position, co,
                          poly.layer);
            }
        }
        triangle_type_current = Triangle::Type::NONE;
    }
    else { // normal polygon
        triangle_type_current = Triangle::Type::POLYGON;
        TPPLPoly po;
        po.Init(poly.vertices.size());
        po.SetHole(false);
        {
            size_t i = 0;
            for (auto &it : poly.vertices) {
                po[i].x = it.position.x;
                po[i].y = it.position.y;
                i++;
            }
        }
        std::list<TPPLPoly> outpolys;
        TPPLPartition part;
        po.SetOrientation(TPPL_CCW);
        part.Triangulate_EC(&po, &outpolys);
        for (auto &tri : outpolys) {
            assert(tri.GetNumPoints() == 3);
            Coordf p0 = transform.transform(coordf_from_pt(tri[0]));
            Coordf p1 = transform.transform(coordf_from_pt(tri[1]));
            Coordf p2 = transform.transform(coordf_from_pt(tri[2]));
            add_triangle(poly.layer, p0, p1, p2, co);
        }
        for (size_t i = 0; i < poly.vertices.size(); i++) {
            draw_line(poly.vertices[i].position, poly.vertices[(i + 1) % poly.vertices.size()].position, co,
                      poly.layer);
        }
        triangle_type_current = Triangle::Type::NONE;
    }

    if (interactive) {
        const Polygon::Vertex *v_last = nullptr;
        size_t i = 0;
        for (const auto &it : ipoly.vertices) {
            if (v_last) {
                auto center = (v_last->position + it.position) / 2;
                if (v_last->type != Polygon::Vertex::Type::ARC) {
                    selectables.append_line(poly.uuid, ObjectType::POLYGON_EDGE, v_last->position, it.position, 0,
                                            i - 1, poly.layer);

                    targets.emplace_back(poly.uuid, ObjectType::POLYGON_EDGE, center, i - 1, poly.layer);
                }
            }
            selectables.append(poly.uuid, ObjectType::POLYGON_VERTEX, it.position, i, poly.layer);
            targets.emplace_back(poly.uuid, ObjectType::POLYGON_VERTEX, it.position, i, poly.layer);
            if (it.type == Polygon::Vertex::Type::ARC) {
                selectables.append(poly.uuid, ObjectType::POLYGON_ARC_CENTER, it.arc_center, i, poly.layer);
                targets.emplace_back(poly.uuid, ObjectType::POLYGON_ARC_CENTER, it.arc_center, i, poly.layer);
            }
            v_last = &it;
            i++;
        }
        if (ipoly.vertices.back().type != Polygon::Vertex::Type::ARC) {
            auto center = (ipoly.vertices.front().position + ipoly.vertices.back().position) / 2;
            targets.emplace_back(poly.uuid, ObjectType::POLYGON_EDGE, center, i - 1, poly.layer);
            selectables.append_line(poly.uuid, ObjectType::POLYGON_EDGE, poly.vertices.front().position,
                                    ipoly.vertices.back().position, 0, i - 1, poly.layer);
        }
    }
}

void Canvas::render(const Shape &shape, bool interactive)
{
    if (img_mode) {
        img_polygon(shape.to_polygon());
        return;
    }
    if (interactive) {
        auto bb = shape.get_bbox();
        selectables.append(shape.uuid, ObjectType::SHAPE, shape.placement.shift, shape.placement.transform(bb.first),
                           shape.placement.transform(bb.second), 0, shape.layer);
        targets.emplace_back(shape.uuid, ObjectType::SHAPE, shape.placement.shift, 0, shape.layer);
    }
    if (shape.form == Shape::Form::CIRCLE) {
        auto r = shape.params.at(0);
        transform_save();
        transform.accumulate(shape.placement);
        draw_line(Coordf(0, 0), Coordf(.1e3, 0), ColorP::FROM_LAYER, shape.layer, true, r);
        transform_restore();
    }
    else if (shape.form == Shape::Form::RECTANGLE) {
        transform_save();
        transform.accumulate(shape.placement);
        auto w = shape.params.at(0);
        auto h = shape.params.at(1);
        add_triangle(shape.layer, transform.transform(Coordf(-w / 2, 0)), transform.transform(Coordf(w / 2, 0)),
                     Coordf(h, NAN), ColorP::FROM_LAYER, Triangle::FLAG_BUTT);
        transform_restore();
    }
    else if (shape.form == Shape::Form::OBROUND) {
        transform_save();
        transform.accumulate(shape.placement);
        auto w = shape.params.at(0);
        auto h = shape.params.at(1);
        draw_line(Coordf(-w / 2 + h / 2, 0), Coordf(w / 2 - h / 2, 0), ColorP::FROM_LAYER, shape.layer, true, h);
        transform_restore();
    }
    else {
        Polygon poly = shape.to_polygon();
        render(poly, false);
    }
}

void Canvas::render(const Hole &hole, bool interactive)
{
    auto co = ColorP::HOLE;
    img_hole(hole);
    if (img_mode)
        return;

    transform_save();
    transform.accumulate(hole.placement);
    int64_t d = hole.diameter / 2;
    int64_t l = std::max((int64_t)hole.length / 2 - d, (int64_t)0);
    if (hole.shape == Hole::Shape::ROUND) {
        draw_line(Coordf(), Coordf(100, 0), co, 10000, true, hole.diameter);
        if (hole.plated) {
            draw_line(Coordf(), Coordf(100, 0), co, 10000, true, hole.diameter * 0.9);
        }
        float x = hole.diameter / 2 / M_SQRT2;
        draw_line(Coordi(-x, -x), Coordi(x, x), co);
        draw_line(Coordi(x, -x), Coordi(-x, x), co);
        if (interactive)
            selectables.append(hole.uuid, ObjectType::HOLE, Coordi(), Coordi(-d, -d), Coordi(d, d));
    }
    else if (hole.shape == Hole::Shape::SLOT) {
        draw_arc(Coordi(-l, 0), d, 0, 2 * M_PI, co);
        draw_arc(Coordi(l, 0), d, 0, 2 * M_PI, co);
        draw_line(Coordi(-l, -d), Coordi(l, -d), co);
        draw_line(Coordi(-l, d), Coordi(l, d), co);
        if (interactive)
            selectables.append(hole.uuid, ObjectType::HOLE, Coordi(), Coordi(-l - d, -d), Coordi(l + d, +d));
    }
    if (interactive)
        targets.emplace_back(hole.uuid, ObjectType::HOLE, hole.placement.shift);
    transform_restore();
}

void Canvas::render(const Pad &pad)
{
    transform_save();
    transform.accumulate(pad.placement);
    img_net(pad.net);
    if (pad.padstack.type == Padstack::Type::THROUGH)
        img_patch_type(PatchType::PAD_TH);
    else
        img_patch_type(PatchType::PAD);
    render(pad.padstack, false);
    img_patch_type(PatchType::OTHER);
    img_net(nullptr);
    transform_restore();
}

void Canvas::render(const Symbol &sym, bool on_sheet, bool smashed, ColorP co)
{
    if (!on_sheet) {
        for (const auto &it : sym.junctions) {
            auto &junc = it.second;
            selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, 10000, true);
            targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position));
        }
    }

    for (const auto &it : sym.lines) {
        render(it.second, !on_sheet, co);
    }

    if (object_refs_current.size() && object_refs_current.back().type == ObjectType::SCHEMATIC_SYMBOL) {
        auto sym_uuid = object_refs_current.back().uuid;
        for (const auto &it : sym.pins) {
            object_refs_current.emplace_back(ObjectType::SYMBOL_PIN, it.second.uuid, sym_uuid);
            render(it.second, !on_sheet, co);
            object_refs_current.pop_back();
        }
    }
    else {
        for (const auto &it : sym.pins) {
            render(it.second, !on_sheet, co);
        }
    }

    for (const auto &it : sym.arcs) {
        render(it.second, !on_sheet, co);
    }
    for (const auto &it : sym.polygons) {
        render(it.second, !on_sheet, co);
    }
    if (!smashed) {
        for (const auto &it : sym.texts) {
            render(it.second, !on_sheet, co);
        }
    }
}

void Canvas::render(const Sheet &sheet)
{
    if (sheet.frame.uuid) {
        render(sheet.frame, true);
    }
    for (const auto &it : sheet.junctions) {
        render(it.second, true, ObjectType::SCHEMATIC);
    }
    for (const auto &it : sheet.symbols) {
        render(it.second);
    }
    for (const auto &it : sheet.net_lines) {
        render(it.second);
    }
    for (const auto &it : sheet.texts) {
        if (!it.second.from_smash)
            render(it.second);
    }
    for (const auto &it : sheet.net_labels) {
        render(it.second);
    }
    for (const auto &it : sheet.power_symbols) {
        render(it.second);
    }
    for (const auto &it : sheet.warnings) {
        render(it);
    }
    for (const auto &it : sheet.bus_labels) {
        render(it.second);
    }
    for (const auto &it : sheet.bus_rippers) {
        render(it.second);
    }
    for (const auto &it : sheet.lines) {
        render(it.second);
    }
    for (const auto &it : sheet.arcs) {
        render(it.second);
    }
    for (const auto &it : sheet.pictures) {
        render(it.second);
    }
}

void Canvas::render(const Padstack &padstack, bool interactive)
{
    for (const auto &it : padstack.holes) {
        render(it.second, interactive);
    }
    img_padstack(padstack);
    img_set_padstack(true);
    for (const auto &it : padstack.polygons) {
        render(it.second, interactive);
    }
    for (const auto &it : padstack.shapes) {
        render(it.second, interactive);
    }
    img_set_padstack(false);
}

void Canvas::render_pad_overlay(const Pad &pad)
{
    if (img_mode)
        return;
    transform_save();
    transform.accumulate(pad.placement);
    auto bb = pad.padstack.get_bbox(true); // only copper
    auto a = bb.first;
    auto b = bb.second;

    auto pad_width = abs(b.x - a.x);
    auto pad_height = abs(b.y - a.y);

    std::set<int> text_layers;
    switch (pad.padstack.type) {
    case Padstack::Type::TOP:
        text_layers.emplace(get_overlay_layer(BoardLayers::TOP_COPPER, true));
        break;

    case Padstack::Type::BOTTOM:
        text_layers.emplace(get_overlay_layer(BoardLayers::BOTTOM_COPPER, true));
        break;

    default:
        text_layers.emplace(get_overlay_layer(BoardLayers::TOP_COPPER, true));
        text_layers.emplace(get_overlay_layer(BoardLayers::BOTTOM_COPPER, true));
    }

    Placement tr = transform;
    if (get_flip_view()) {
        tr.shift.x *= -1;
        tr.invert_angle();
    }

    set_lod_size(std::min(pad_height, pad_width));
    for (const auto overlay_layer : text_layers) {
        if (pad.secondary_text.size()) {
            draw_bitmap_text_box(tr, pad_width, pad_height, pad.name, ColorP::TEXT_OVERLAY, overlay_layer,
                                 TextBoxMode::UPPER);
            draw_bitmap_text_box(tr, pad_width, pad_height, pad.secondary_text, ColorP::TEXT_OVERLAY, overlay_layer,
                                 TextBoxMode::LOWER);
        }
        else {
            draw_bitmap_text_box(tr, pad_width, pad_height, pad.name, ColorP::TEXT_OVERLAY, overlay_layer,
                                 TextBoxMode::FULL);
        }
    }
    set_lod_size(-1);
    transform_restore();
}

void Canvas::render(const Package &pkg, bool interactive, bool smashed, bool omit_silkscreen, bool omit_outline)
{
    if (interactive) {
        for (const auto &it : pkg.junctions) {
            auto &junc = it.second;
            selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, junc.layer);
            targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position), 0, junc.layer);
        }
    }

    for (const auto &it : pkg.lines) {
        if (omit_silkscreen && BoardLayers::is_silkscreen(it.second.layer))
            continue;
        render(it.second, interactive);
    }
    for (const auto &it : pkg.texts) {
        if (omit_silkscreen && BoardLayers::is_silkscreen(it.second.layer))
            continue;

        if (!smashed || !BoardLayers::is_silkscreen(it.second.layer))
            render(it.second, interactive);
    }
    for (const auto &it : pkg.arcs) {
        if (omit_silkscreen && BoardLayers::is_silkscreen(it.second.layer))
            continue;
        render(it.second, interactive);
    }
    if (object_refs_current.size() && object_refs_current.back().type == ObjectType::BOARD_PACKAGE) {
        auto pkg_uuid = object_refs_current.back().uuid;
        for (const auto &it : pkg.pads) {
            object_refs_current.emplace_back(ObjectType::PAD, it.second.uuid, pkg_uuid);
            render_pad_overlay(it.second);
            render(it.second);
            object_refs_current.pop_back();
        }
    }
    else if (interactive) {
        for (const auto &it : pkg.pads) {
            object_refs_current.emplace_back(ObjectType::PAD, it.second.uuid);
            render_pad_overlay(it.second);
            render(it.second);
            object_refs_current.pop_back();
        }
    }
    else {
        for (const auto &it : pkg.pads) {
            render_pad_overlay(it.second);
            render(it.second);
        }
    }
    for (const auto &it : pkg.polygons) {
        if (omit_silkscreen && BoardLayers::is_silkscreen(it.second.layer))
            continue;
        if (omit_outline && it.second.layer == BoardLayers::L_OUTLINE)
            continue;
        render(it.second, interactive);
    }

    if (interactive) {
        for (const auto &it : pkg.pads) {
            transform_save();
            transform.accumulate(it.second.placement);
            auto bb = it.second.padstack.get_bbox();
            selectables.append(it.second.uuid, ObjectType::PAD, {0, 0}, bb.first, bb.second);
            transform_restore();
            targets.emplace_back(it.second.uuid, ObjectType::PAD, it.second.placement.shift);
        }
        for (const auto &it : pkg.warnings) {
            render(it);
        }
        for (const auto &it : pkg.dimensions) {
            render(it.second);
        }
        for (const auto &it : pkg.pictures) {
            render(it.second);
        }
    }
}

void Canvas::render(const Buffer &buf)
{
    for (const auto &it : buf.junctions) {
        render(it.second);
    }
    for (const auto &it : buf.pins) {
        render(it.second);
    }
    for (const auto &it : buf.holes) {
        render(it.second);
    }
    for (const auto &it : buf.lines) {
        render(it.second);
    }
    for (const auto &it : buf.texts) {
        render(it.second);
    }
    for (const auto &it : buf.arcs) {
        render(it.second);
    }
    for (const auto &it : buf.pads) {
        render(it.second);
    }
    for (const auto &it : buf.symbols) {
        render(it.second);
    }
    for (const auto &it : buf.net_lines) {
        render(it.second);
    }
    for (const auto &it : buf.power_symbols) {
        render(it.second);
    }
    for (const auto &it : buf.net_labels) {
        render(it.second);
    }
}

void Canvas::render(const BoardPackage &pkg, bool interactive)
{
    transform_save();
    transform.accumulate(pkg.placement);
    auto bb = pkg.package.get_bbox();
    if (pkg.flip) {
        transform.invert_angle();
    }
    if (interactive) {
        selectables.append(pkg.uuid, ObjectType::BOARD_PACKAGE, {0, 0}, bb.first, bb.second, 0,
                           pkg.flip ? BoardLayers::BOTTOM_PACKAGE : BoardLayers::TOP_PACKAGE);
        targets.emplace_back(pkg.uuid, ObjectType::BOARD_PACKAGE, pkg.placement.shift);

        for (const auto &it : pkg.package.pads) {
            targets.emplace_back(UUIDPath<2>(pkg.uuid, it.first), ObjectType::PAD,
                                 transform.transform(it.second.placement.shift));
        }
    }
    if (interactive)
        object_refs_current.emplace_back(ObjectType::BOARD_PACKAGE, pkg.uuid);

    render(pkg.package, false, pkg.smashed, pkg.omit_silkscreen, pkg.omit_outline);

    if (interactive)
        object_refs_current.pop_back();

    transform.reset();
    transform_restore();
}

void Canvas::render(const Via &via, bool interactive)
{
    transform_save();
    {
        Placement pl;
        pl.shift = via.junction->position;
        transform.accumulate(pl);
    }
    auto bb = via.padstack.get_bbox();
    if (interactive)
        selectables.append(via.uuid, ObjectType::VIA, {0, 0}, bb.first, bb.second);
    img_net(via.junction->net);
    img_patch_type(PatchType::VIA);
    if (interactive)
        object_refs_current.emplace_back(ObjectType::VIA, via.uuid);
    render(via.padstack, false);
    if (via.locked) {
        auto ol = get_overlay_layer(BoardLayers::TOP_COPPER);
        draw_lock({0, 0}, 0.7 * std::min(std::abs(bb.second.x - bb.first.x), std::abs(bb.second.y - bb.first.y)),
                  ColorP::TEXT_OVERLAY, ol, true);
    }
    if (show_text_in_vias && interactive && via.junction->net && via.junction->net->name.size()) {
        auto size = (bb.second.x - bb.first.x) * 1.2;
        set_lod_size(size);
        Placement p(via.junction->position);
        draw_bitmap_text_box(p, size, size, via.junction->net->name, ColorP::TEXT_OVERLAY, 10000, TextBoxMode::FULL);
        set_lod_size(-1);
    }


    if (interactive)
        object_refs_current.pop_back();
    img_net(nullptr);
    img_patch_type(PatchType::OTHER);
    transform_restore();
}

void Canvas::render(const BoardHole &hole, bool interactive)
{
    transform_save();
    transform.accumulate(hole.placement);
    auto bb = hole.padstack.get_bbox();
    if (interactive)
        selectables.append(hole.uuid, ObjectType::BOARD_HOLE, {0, 0}, bb.first, bb.second);
    img_net(hole.net);
    if (hole.padstack.type == Padstack::Type::HOLE)
        img_patch_type(PatchType::HOLE_PTH);
    else
        img_patch_type(PatchType::HOLE_NPTH);
    if (interactive)
        object_refs_current.emplace_back(ObjectType::BOARD_HOLE, hole.uuid);
    render(hole.padstack, false);
    if (interactive)
        object_refs_current.pop_back();
    img_net(nullptr);
    img_patch_type(PatchType::OTHER);
    transform_restore();
}

void Canvas::render(const class Dimension &dim)
{
    typedef Coord<double> Coordd;

    Coordd p0 = dim.p0;
    Coordd p1 = dim.p1;

    if (dim.mode == Dimension::Mode::HORIZONTAL) {
        p1 = Coordd(p1.x, p0.y);
    }
    else if (dim.mode == Dimension::Mode::VERTICAL) {
        p1 = Coordd(p0.x, p1.y);
    }

    Coordd v = p1 - p0;
    auto vn = v / sqrt(v.mag_sq());
    Coordd w = Coordd(-v.y, v.x);
    if (dim.mode == Dimension::Mode::HORIZONTAL) {
        w.y = std::abs(w.y);
    }
    else if (dim.mode == Dimension::Mode::VERTICAL) {
        w.x = std::abs(w.x);
    }
    auto wn = w / sqrt(w.mag_sq());

    ColorP co = ColorP::DIMENSION;

    if (v.mag_sq()) {

        auto q0 = p0 + wn * dim.label_distance;
        auto q1 = p1 + wn * dim.label_distance;

        draw_line(dim.p0, p0 + wn * (dim.label_distance + sgn(dim.label_distance) * (int64_t)dim.label_size / 2), co,
                  10000, true, 0);
        draw_line(dim.p1, p1 + wn * (dim.label_distance + sgn(dim.label_distance) * (int64_t)dim.label_size / 2), co,
                  10000, true, 0);

        auto length = sqrt(v.mag_sq());
        auto s = dim_to_string(length, false);

        auto text_bb = draw_text0({0, 0}, dim.label_size, s, 0, false, TextOrigin::CENTER, co, 10000, 0, false);
        auto text_width = std::abs(text_bb.second.x - text_bb.first.x);

        Coordf text_pos = (q0 + v / 2) - (vn * text_width / 2);
        auto angle = atan2(v.y, v.x);

        if ((text_width + 2 * dim.label_size) > length) {
            text_pos = q1;
            draw_line(q0, q1, co, 10000, true, 0);
        }
        else {
            draw_line(q0, (q0 + v / 2) - (vn * (text_width / 2 + .5 * dim.label_size)), co, 10000, true, 0);
            draw_line(q1, (q0 + v / 2) + (vn * (text_width / 2 + .5 * dim.label_size)), co, 10000, true, 0);
        }

        float arrow_mul;
        if (length > dim.label_size * 2) {
            arrow_mul = 1;
        }
        else {
            text_pos = q1 + vn * dim.label_size;
            arrow_mul = -1;
        }
        transform_save();
        transform.reset();
        transform.shift = Coordi(q0.x, q0.y);
        transform.set_angle_rad(angle);
        draw_line({0, 0}, Coordf(arrow_mul * dim.label_size, dim.label_size / 2), co, 10000, true, 0);
        draw_line({0, 0}, Coordf(arrow_mul * dim.label_size, (int64_t)dim.label_size / -2), co, 10000, true, 0);

        transform.shift = Coordi(q1.x, q1.y);
        draw_line({0, 0}, Coordf(-arrow_mul * dim.label_size, dim.label_size / 2), co, 10000, true, 0);
        draw_line({0, 0}, Coordf(-arrow_mul * dim.label_size, (int64_t)dim.label_size / -2), co, 10000, true, 0);
        transform_restore();

        int angle_i = (angle / M_PI) * 32768;
        auto real_text_bb = draw_text0(Coordi(text_pos.x, text_pos.y), dim.label_size, s,
                                       get_flip_view() ? (32768 - angle_i) : angle_i, get_flip_view(),
                                       TextOrigin::CENTER, co, 10000, 0, true);

        selectables.append(dim.uuid, ObjectType::DIMENSION, q0, real_text_bb.first, real_text_bb.second, 2);
        targets.emplace_back(dim.uuid, ObjectType::DIMENSION, Coordi(q0.x, q0.y), 2);
    }

    selectables.append(dim.uuid, ObjectType::DIMENSION, dim.p0, 0);
    selectables.append(dim.uuid, ObjectType::DIMENSION, dim.p1, 1);

    targets.emplace_back(dim.uuid, ObjectType::DIMENSION, dim.p0, 0);
    targets.emplace_back(dim.uuid, ObjectType::DIMENSION, dim.p1, 1);
}

void Canvas::render(const Board &brd, bool interactive, PanelMode mode, OutlineMode outline_mode)
{
    clock_t begin = clock();

    for (const auto &it : brd.holes) {
        render(it.second, interactive);
    }
    if (interactive) {
        for (const auto &it : brd.junctions) {
            render(it.second, true, ObjectType::BOARD);
        }
    }
    if (!img_mode && interactive) {
        for (const auto &it_net : brd.airwires) {
            if (airwire_filter == nullptr || airwire_filter->airwire_is_visible(it_net.first)) {
                object_refs_current.emplace_back(ObjectType::AIRWIRE, it_net.first);
                for (const auto &it : it_net.second) {
                    render(it);
                }
                object_refs_current.pop_back();
            }
        }
    }
    for (const auto &it : brd.polygons) {
        if (outline_mode == OutlineMode::OMIT && it.second.layer == BoardLayers::L_OUTLINE)
            continue;
        render(it.second, interactive);
    }
    for (const auto &it : brd.texts) {
        render(it.second, interactive);
    }
    for (const auto &it : brd.tracks) {
        render(it.second, interactive);
    }
    for (const auto &it : brd.packages) {
        render(it.second, interactive);
    }
    for (const auto &it : brd.vias) {
        render(it.second, interactive);
    }
    for (const auto &it : brd.lines) {
        render(it.second, interactive);
    }
    for (const auto &it : brd.arcs) {
        render(it.second, interactive);
    }
    if (mode == PanelMode::INCLUDE) {
        for (const auto &it : brd.board_panels) {
            render(it.second);
        }
    }

    if (interactive) {
        for (const auto &it : brd.dimensions) {
            render(it.second);
        }
        for (const auto &it : brd.connection_lines) {
            render(it.second);
        }

        for (const auto &path : brd.obstacles) {
            for (auto it = path.cbegin(); it < path.cend(); it++) {
                if (it != path.cbegin()) {
                    auto b = it - 1;
                    draw_line(Coordf(b->X, b->Y), Coordf(it->X, it->Y), ColorP::AIRWIRE, 10000, false, 0);
                }
            }
        }

        unsigned int i = 0;
        for (auto it = brd.track_path.cbegin(); it < brd.track_path.cend(); it++) {
            if (it != brd.track_path.cbegin()) {
                auto b = it - 1;
                draw_line(Coordf(b->X, b->Y), Coordf(it->X, it->Y), ColorP::AIRWIRE, 10000, false, 0);
            }
            if (i % 2 == 0) {
                draw_line(Coordf(it->X, it->Y), Coordf(it->X, it->Y), ColorP::AIRWIRE, 10000, false, .1_mm);
            }
            i++;
        }
        for (const auto &it : brd.warnings) {
            render(it);
        }
        for (const auto &it : brd.pictures) {
            render(it.second);
        }
    }

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "render took " << 1 / elapsed_secs << std::endl;
}

void Canvas::render(const BoardPanel &panel)
{
    if (!panel.included_board->is_valid()) {
        draw_error(panel.placement.shift, 2e5, "invalid board", false);
        return;
    }
    transform_save();
    transform.accumulate(panel.placement);
    auto bb = panel.included_board->board->get_bbox();
    selectables.append(panel.uuid, ObjectType::BOARD_PANEL, {0, 0}, bb.first, bb.second, 0, 10000);
    render(*panel.included_board->board, false, PanelMode::SKIP,
           panel.omit_outline ? OutlineMode::OMIT : OutlineMode::INCLUDE);
    transform_restore();
}

void Canvas::render(const Frame &fr, bool on_sheet)
{
    if (!on_sheet) {
        for (const auto &it : fr.junctions) {
            auto &junc = it.second;
            selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, 10000, true);
            targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position));
        }
    }
    for (const auto &it : fr.lines) {
        render(it.second, !on_sheet);
    }


    for (const auto &it : fr.arcs) {
        render(it.second, !on_sheet);
    }
    for (const auto &it : fr.polygons) {
        render(it.second, !on_sheet);
    }
    for (const auto &it : fr.texts) {
        render(it.second, !on_sheet);
    }
    auto c = ColorP::FRAME;
    draw_line(Coordf(), Coordf(fr.width, 0), c);
    draw_line(Coordf(fr.width, 0), Coordf(fr.width, fr.height), c);
    draw_line(Coordf(fr.width, fr.height), Coordf(0, fr.height), c);
    draw_line(Coordf(0, fr.height), Coordf(), c);
}

void Canvas::render(const Picture &pic)
{
    if (!pic.data && !img_mode) {
        draw_error(pic.placement.shift, 2e5, "Image " + (std::string)pic.data_uuid + " not found");
        selectables.append_angled(pic.uuid, ObjectType::PICTURE, pic.placement.shift, pic.placement.shift,
                                  Coordf(1_mm, 1_mm), 0);
        return;
    }
    pictures.emplace_back();
    auto &x = pictures.back();
    x.angle = pic.placement.get_angle() / 65536.0 * 2 * M_PI;
    x.x = pic.placement.shift.x;
    x.y = pic.placement.shift.y;
    x.px_size = pic.px_size;
    x.data = pic.data;
    x.on_top = pic.on_top;
    x.opacity = pic.opacity;
    float w = pic.data->width * pic.px_size;
    float h = pic.data->height * pic.px_size;
    if (img_mode)
        return;
    selectables.append_angled(pic.uuid, ObjectType::PICTURE, pic.placement.shift, pic.placement.shift, {w, h},
                              pic.placement.get_angle_rad());
}

} // namespace horizon
