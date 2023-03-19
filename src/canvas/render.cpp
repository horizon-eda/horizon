#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "board/track.hpp"
#include "canvas.hpp"
#include "common/hole.hpp"
#include "common/polygon.hpp"
#include "common/text.hpp"
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
#include "pool/decal.hpp"
#include "selectables.hpp"
#include "selection_filter.hpp"
#include "target.hpp"
#include "util/placement.hpp"
#include "util/geom_util.hpp"
#include "board/board_panel.hpp"
#include "common/picture.hpp"
#include "block_symbol/block_symbol.hpp"
#include "util/polygon_arc_removal_proxy.hpp"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <sstream>

namespace horizon {

void Canvas::render(const Junction &junc, bool interactive, ObjectType mode)
{
    ColorP c = ColorP::JUNCTION;
    bool draw = true;

    if (mode == ObjectType::BOARD)
        draw = false;

    object_ref_push(ObjectType::JUNCTION, junc.uuid);
    if (draw) {
        draw_cross(junc.position, 0.25_mm, c);
    }
    object_ref_pop();

    if (interactive) {
        selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, junc.layer);
        targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position), 0, junc.layer);
    }
}

void Canvas::render(const SchematicJunction &junc)
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
    object_ref_push(ObjectType::JUNCTION, junc.uuid);

    auto connection_count =
            junc.connected_net_lines.size() + junc.connected_power_symbols.size() + junc.connected_net_ties.size();
    if (connection_count == 2) {
        if (show_all_junctions_in_schematic)
            draw_plus(junc.position, 250000, c);
    }
    else if (connection_count >= 3) {
        draw_line(junc.position, junc.position + Coordi(0, 1000), c, 0, true, 0.75_mm);
        img_line(junc.position, junc.position + Coordi(0, 1000), 0.75_mm, 0, true);
    }
    else if (junc.connected_bus_labels.size() || junc.connected_bus_rippers.size() || junc.connected_net_labels.size()
             || junc.connected_lines.size() || junc.connected_arcs.size() || junc.connected_net_ties.size()) {
        // nop
    }
    else {
        draw_cross(junc.position, 0.25_mm, c);
    }
    object_ref_pop();

    selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, junc.layer);
    targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position), 0, junc.layer);
}

void Canvas::render(const PowerSymbol &sym)
{
    auto c = ColorP::FROM_LAYER;
    transform.shift = sym.junction->position;
    object_ref_push(ObjectType::POWER_SYMBOL, sym.uuid);

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
            draw_text(sym.junction->position + text_offset, 1.5_mm, sym.junction->net->name, text_angle,
                      TextOrigin::CENTER, c, 0, {});
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
            draw_circle({0, 1.75_mm}, 0.75_mm, ColorP::FROM_LAYER, 0);
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
            draw_text(sym.junction->position + text_offset, 1.5_mm, sym.junction->net->name, text_angle,
                      TextOrigin::CENTER, c, 0, {});
    }
    }

    transform.reset();
    img_auto_line = false;

    object_ref_pop();
}

ObjectRef Canvas::add_line(const std::deque<Coordi> &pts, int64_t width, ColorP color, int layer)
{
    if (pts.size() < 2) {
        return ObjectRef();
    }
    auto uu = UUID::random();
    ObjectRef sr(ObjectType::LINE, uu);
    object_ref_push(sr);
    for (size_t i = 1; i < pts.size(); i++) {
        auto pt1 = pts.at(i - 1);
        auto pt2 = pts.at(i);
        draw_line(pt1, pt2, color, layer, false, width);
    }
    object_ref_pop();
    request_push();
    return sr;
}

ObjectRef Canvas::add_arc(const Coordi &from, const Coordi &to, const Coordi &center, int64_t width, ColorP color,
                          int layer)
{
    auto uu = UUID::random();
    ObjectRef sr(ObjectType::ARC, uu);
    object_ref_push(sr);
    draw_arc(from, to, center, color, layer, width);
    object_ref_pop();
    request_push();
    return sr;
}

void Canvas::render(const Line &line, bool interactive, ColorP co)
{
    img_line(line.from->position, line.to->position, line.width, line.layer);
    if (img_mode)
        return;
    triangle_type_current = TriangleInfo::Type::GRAPHICS;
    draw_line(line.from->position, line.to->position, co, line.layer, true, line.width);
    triangle_type_current = TriangleInfo::Type::NONE;
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
    object_ref_push(ObjectType::LINE_NET, line.uuid);
    draw_line(line.from.get_position(), line.to.get_position(), c, 0, true, width);
    object_ref_pop();
    selectables.append_line(line.uuid, ObjectType::LINE_NET, line.from.get_position(), line.to.get_position(), width);
}

static std::string get_name(const std::string &n)
{
    if (n.size())
        return n;
    else
        return "unnamed net";
}

void Canvas::render(const SchematicNetTie &tie)
{
    const auto from = tie.from->position;
    const auto to = tie.to->position;

    object_ref_push(ObjectType::SCHEMATIC_NET_TIE, tie.uuid);
    // draw_line(from, to, ColorP::NET_TIE, 0, true, width);
    Coordf p0;
    Coordf p1;
    if (from < to) {
        p0 = from;
        p1 = to;
    }
    else {
        p0 = to;
        p1 = from;
    }
    const auto v = p1 - p0;
    const auto vn = v.normalize();
    const auto center = ((p0 + p1) / 2);
    const auto perp_n = Coordf(v.y, -v.x).normalize();
    const float h = .5_mm;
    const auto s = v.mag();
    const auto r = (4 * h * h + s * s) / (8 * h);
    const auto d = r - h;
    const auto arc_center = center + (perp_n * d);
    img_auto_line = img_mode;
    draw_arc(p1, p0, arc_center, ColorP::NET_TIE, 0, 0);
    draw_arc(p0, p1, center - (perp_n * d), ColorP::NET_TIE, 0, 0);

    const auto text_pos = center + (perp_n * 1.5_mm);

    TextRenderer::Options opts;
    opts.center = true;
    draw_text(text_pos, 1.5_mm,
              get_name(tie.net_tie->net_primary->name) + "\n" + get_name(tie.net_tie->net_secondary->name),
              angle_from_rad(v.angle()), TextOrigin::CENTER, ColorP::NET_TIE, 0, opts);
    img_auto_line = false;
    object_ref_pop();
    if (img_mode)
        return;
    selectables.append_line(tie.uuid, ObjectType::SCHEMATIC_NET_TIE, p0 + (vn * h), p1 - (vn * h), h * 2);
}

void Canvas::render(const Track &track, bool interactive)
{
    auto color = ColorP::FROM_LAYER;
    if (track.net == nullptr) {
        color = ColorP::BUS;
    }
    auto width = track.width;
    if (interactive)
        object_ref_push(ObjectType::TRACK, track.uuid);
    if (img_mode) {
        img_net(track.net);
        img_patch_type(PatchType::TRACK);
        if (track.is_arc()) {
            img_arc(track.from.get_position(), track.to.get_position(), track.center.value(), track.width, track.layer);
        }
        else {
            img_line(track.from.get_position(), track.to.get_position(), width, track.layer);
        }
    }
    img_patch_type(PatchType::OTHER);
    img_net(nullptr);
    if (interactive)
        object_ref_pop();
    if (img_mode)
        return;
    auto layer = track.layer;

    auto center = (track.from.get_position() + track.to.get_position()) / 2;
    if (interactive)
        object_ref_push(ObjectType::TRACK, track.uuid);

    if (track.is_arc()) {
        draw_arc(track.from.get_position(), track.to.get_position(), track.center.value(), color, layer, width);
    }
    else {
        draw_line(track.from.get_position(), track.to.get_position(), color, layer, true, width);
    }
    if (track.locked) {
        auto ol = get_overlay_layer(layer);
        draw_lock(center, 0.7 * track.width, ColorP::TEXT_OVERLAY, ol, true);
    }
    if (interactive && show_text_in_tracks && width > 0 && track.net && track.net->name.size() && track.from.is_junc()
        && track.to.is_junc() && (!show_text_in_vias || (!track.from.junc->has_via && !track.to.junc->has_via))
        && !track.center) {
        auto overlay_layer = get_overlay_layer(track.layer, true);
        set_lod_size(width);
        const auto vec = (track.from.get_position() - track.to.get_position());
        const auto length = vec.magd();
        Placement p;
        p.set_angle_rad(get_view_angle());
        if (get_flip_view())
            p.invert_angle();
        p.accumulate(Placement(center));
        p.set_angle_rad(p.get_angle_rad() + vec.angle());
        if (get_flip_view()) {
            p.shift.x *= -1;
            p.invert_angle();
        }
        draw_bitmap_text_box(p, length, width, track.net->name, ColorP::TEXT_OVERLAY, overlay_layer, TextBoxMode::FULL);
        set_lod_size(-1);
    }
    if (interactive)
        object_ref_pop();
    if (interactive) {
        bool force_line = false;
        if (track.is_arc()) {
            Coordf a(track.from.get_position()); // ,b,c;
            Coordf b(track.to.get_position());   // ,b,c;
            const auto arc_center = project_onto_perp_bisector(a, b, track.center.value());
            const float arc_radius = (arc_center - a).mag();
            const auto arc_a0 = c2pi(atan2f(a.y - arc_center.y, a.x - arc_center.x));
            const auto arc_a1 = c2pi(atan2f(b.y - arc_center.y, b.x - arc_center.x));
            const float ax = std::min(asin(track.width / 2 / arc_radius),
                                      static_cast<float>((2 * M_PI - c2pi(arc_a1 - arc_a0)) / 2 - .1e-4));
            const float dphi = c2pi(arc_a1 - arc_a0);
            auto mid = arc_center + Coordf::euler(arc_radius, arc_a0 + dphi / 2);
            const auto ri = arc_radius - track.width / 2;
            if (ri > 0)
                selectables.append_arc_midpoint(track.uuid, ObjectType::TRACK, mid, ri, arc_radius + track.width / 2,
                                                arc_a0 - ax, arc_a1 + ax, 0, track.layer);
            else
                force_line = true;
        }

        if (!track.is_arc() || force_line) {
            selectables.append_line(track.uuid, ObjectType::TRACK, track.from.get_position(), track.to.get_position(),
                                    track.width, 0, track.layer);
        }
    }
}

void Canvas::render(const BoardNetTie &tie, bool interactive)
{
    auto c = ColorP::FROM_LAYER;
    auto width = tie.width;


    auto layer = tie.layer;

    auto center = (tie.from->position + tie.to->position) / 2;
    if (interactive)
        object_ref_push(ObjectType::BOARD_NET_TIE, tie.uuid);

    if (img_mode) {
        img_net(tie.net_tie->net_primary);
        img_patch_type(PatchType::NET_TIE);
        img_line(tie.from->position, tie.to->position, width, tie.layer);
        img_patch_type(PatchType::OTHER);
        img_net(nullptr);
    }

    draw_line(tie.from->position, tie.to->position, c, layer, true, width);
    if (interactive && show_text_in_tracks && width > 0) {
        auto overlay_layer = get_overlay_layer(tie.layer, true);
        set_lod_size(width);
        const auto vec = (tie.from->position - tie.to->position);
        const auto length = vec.magd();
        Placement p;
        p.set_angle_rad(get_view_angle());
        if (get_flip_view())
            p.invert_angle();
        p.accumulate(Placement(center));
        p.set_angle_rad(p.get_angle_rad() + vec.angle());
        if (get_flip_view()) {
            p.shift.x *= -1;
            p.invert_angle();
        }
        draw_bitmap_text_box(p, length, width, tie.net_tie->net_primary->name + "<>" + tie.net_tie->net_secondary->name,
                             ColorP::TEXT_OVERLAY, overlay_layer, TextBoxMode::FULL);
        set_lod_size(-1);
    }
    if (interactive)
        object_ref_pop();
    if (img_mode)
        return;
    if (interactive)
        selectables.append_line(tie.uuid, ObjectType::BOARD_NET_TIE, tie.from->position, tie.to->position, tie.width, 0,
                                tie.layer);
}

void Canvas::render(const ConnectionLine &line)
{
    if (img_mode)
        return;
    draw_line(line.from.get_position(), line.to.get_position(), ColorP::CONNECTION_LINE);
    selectables.append_line(line.uuid, ObjectType::CONNECTION_LINE, line.from.get_position(), line.to.get_position(),
                            0);
}

void Canvas::render(const SymbolPin &pin, SymbolMode mode, ColorP co)
{
    const bool interactive = mode != SymbolMode::SHEET;
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
    if (mode == SymbolMode::EDIT || pin.name_visible) {
        bool draw_in_line = pin.name_orientation == SymbolPin::NameOrientation::IN_LINE
                            || (pin.name_orientation == SymbolPin::NameOrientation::HORIZONTAL
                                && (pin_orientation == Orientation::LEFT || pin_orientation == Orientation::RIGHT));

        if (draw_in_line) {
            draw_text(p_name, 1.5_mm, pin.name, orientation_to_angle(name_orientation), TextOrigin::CENTER, c_name, 0,
                      {});
        }
        else { // draw perp
            Placement tr;
            tr.set_angle(orientation_to_angle(pin_orientation));
            auto shift = tr.transform(Coordi(-1_mm, 0));
            TextRenderer::Options opts;
            opts.center = true;
            draw_text(p_name + shift, 1.5_mm, pin.name, orientation_to_angle(name_orientation) + 16384,
                      TextOrigin::CENTER, c_name, 0, opts);
        }
    }
    std::optional<std::pair<Coordf, Coordf>> pad_extents;
    if (mode == SymbolMode::EDIT || pin.pad_visible) {
        pad_extents = draw_text(p_pad, 0.75_mm, pin.pad, orientation_to_angle(pad_orientation), TextOrigin::BASELINE,
                                c_pad, 0, {});
    }
    if (interactive) {
        if (pad_extents.has_value())
            selectables.append(pin.uuid, ObjectType::SYMBOL_PIN, p0,
                               Coordf::min(pad_extents->first, Coordf::min(p0, p1)),
                               Coordf::max(pad_extents->second, Coordf::max(p0, p1)));
        else
            selectables.append_line(pin.uuid, ObjectType::SYMBOL_PIN, p0, p1, 0);
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
        draw_circle(Coordf(-((int64_t)pin.length) + dot_size / 2, 0), dot_size / 2, c_main, 0);
    }
    if (pin.decoration.clock) {
        draw_line(Coordf(-(int64_t)pin.length, .375_mm), Coordf(-(int64_t)pin.length - .75_mm, 0), c_main, 0, true, 0);
        draw_line(Coordf(-(int64_t)pin.length, -.375_mm), Coordf(-(int64_t)pin.length - .75_mm, 0), c_main, 0, true, 0);
    }
    draw_direction(pin.direction, c_pin);
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
        draw_text(p_nc, 1.5_mm, "NC", orientation_to_angle(nc_orientation), TextOrigin::CENTER, c_pin, 0, {});
        break;

    case SymbolPin::ConnectorStyle::NONE:
        break;
    }
    if (pin.connection_count > 1) {
        draw_line(p0, p0 + Coordi(0, 10), c_main, 0, false, 0.75_mm);
    }
    draw_line(p0, p1, c_main, 0, false);
    img_auto_line = false;
}

void Canvas::render(const Arc &arc, bool interactive, ColorP co)
{
    if (img_mode) {
        img_arc(arc.from->position, arc.to->position, arc.center->position, arc.width, arc.layer);
        return;
    }
    draw_arc(arc.from->position, arc.to->position, arc.center->position, co, arc.layer, arc.width);
    if (interactive) {
        Coordf a(arc.from->position); // ,b,c;
        Coordf b(arc.to->position);   // ,b,c;
        Coordf c = project_onto_perp_bisector(a, b, arc.center->position);
        const float radius0 = (c - a).mag();
        const float a0 = c2pi((a - c).angle());
        const float a1 = c2pi((b - c).angle());
        const float dphi = c2pi(a1 - a0);
        const auto ri = radius0 - arc.width / 2;

        if (ri > 0) {
            const float ax = std::min(asin(arc.width / 2 / radius0), static_cast<float>((2 * M_PI - dphi) / 2 - .1e-4));
            selectables.append_arc(arc.uuid, ObjectType::ARC, c, ri, radius0 + arc.width / 2, a0 - ax, a1 + ax, 0,
                                   arc.layer);
        }
        else {
            selectables.append_line(arc.uuid, ObjectType::ARC, a, b, arc.width, 0, arc.layer);
        }
    }
}

void Canvas::render(const SchematicSymbol &sym)
{
    transform = sym.placement;
    object_ref_push(ObjectType::SCHEMATIC_SYMBOL, sym.uuid);
    render(sym.symbol, SymbolMode::SHEET, sym.smashed, ColorP::FROM_LAYER);
    object_ref_pop();
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

    if (sym.component->nopopulate
        || sym.component->nopopulate_from_instance == Component::NopopulateFromInstance::SET) {
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
    const bool rev = layer_provider->get_layers().at(text.layer).reverse;
    img_patch_type(PatchType::TEXT);
    triangle_type_current = TriangleInfo::Type::TEXT;
    img_text(&text);
    const auto extents = text_renderer.render(text, co, transform, rev);
    img_text(nullptr);
    triangle_type_current = TriangleInfo::Type::NONE;
    img_patch_type(PatchType::OTHER);

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
    const Net *net = label.junction->net;
    if (label.junction->net) {
        txt = label.junction->net->name;
        if (label.junction->net->diffpair)
            c = ColorP::DIFFPAIR;
    }
    if (label.show_port && net && net->is_port) {
        std::string port_txt;
        switch (net->port_direction) {
        case Pin::Direction::BIDIRECTIONAL:
            port_txt = "BIDI";
            break;
        case Pin::Direction::INPUT:
            port_txt = "IN";
            break;
        case Pin::Direction::OUTPUT:
            port_txt = "OUT";
            break;
        case Pin::Direction::OPEN_COLLECTOR:
            port_txt = "OC";
            break;
        case Pin::Direction::POWER_INPUT:
            port_txt = "PIN";
            break;
        case Pin::Direction::POWER_OUTPUT:
            port_txt = "POUT";
            break;
        case Pin::Direction::PASSIVE:
            port_txt = "PASV";
            break;
        case Pin::Direction::NOT_CONNECTED:
            port_txt = "NC";
            break;
        }
        txt = port_txt + ": " + txt;
    }
    if (txt == "") {
        txt = "? plz fix";
    }
    if (label.on_sheets.size() > 0 && label.offsheet_refs) {
        txt += " [" + join(label.on_sheets, ",") + "]";
    }

    object_ref_push(ObjectType::NET_LABEL, label.uuid);
    if (label.style == NetLabel::Style::FLAG) {

        std::pair<Coordf, Coordf> extents;
        Coordi shift;
        std::tie(extents.first, extents.second, shift) =
                draw_flag(label.junction->position, txt, label.size, label.orientation, c);
        selectables.append(label.uuid, ObjectType::NET_LABEL, label.junction->position + shift, extents.first,
                           extents.second);
    }
    else {
        const auto extents = draw_text(label.junction->position, label.size, txt,
                                       orientation_to_angle(label.orientation), TextOrigin::BASELINE, c, 0, {});
        selectables.append(label.uuid, ObjectType::NET_LABEL, label.junction->position + Coordi(0, 1000000),
                           extents.first, extents.second);
    }
    object_ref_pop();
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
    if (ripper.connections.size() < 1) {
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
    const auto extents = draw_text(connector_pos + Coordi(0, 0.5_mm), 1.5_mm, ripper.bus_member->name, angle,
                                   TextOrigin::BASELINE, c, 0, {});
    targets.emplace_back(ripper.uuid, ObjectType::BUS_RIPPER, connector_pos);
    selectables.append(ripper.uuid, ObjectType::BUS_RIPPER, connector_pos, extents.first, extents.second);
}

void Canvas::render(const Warning &warn)
{
    if (img_mode)
        return;
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
    const PolygonArcRemovalProxy arc_removal_proxy{ipoly, 64};
    const auto &poly = arc_removal_proxy.get();
    if (poly.vertices.size() == 0)
        return;

    if (!layer_display.count(poly.layer))
        return;
    if (auto plane = dynamic_cast<Plane *>(poly.usage.ptr)) {
        triangle_type_current = TriangleInfo::Type::PLANE_FILL;
        const auto &tris = fragment_cache.get_triangles(*plane);
        object_ref_push(ObjectType::PLANE, plane->uuid);
        const bool transform_is_identity = transform.is_identity();

        begin_group(poly.layer);
        for (const auto &tri : tris) {
            if (transform_is_identity)
                add_triangle(poly.layer, tri[0], tri[1], tri[2], co);
            else
                add_triangle(poly.layer, transform.transform(tri[0]), transform.transform(tri[1]),
                             transform.transform(tri[2]), co);
        }
        triangle_type_current = TriangleInfo::Type::NONE;


        for (const auto &frag : plane->fragments) {
            ColorP co_orphan = co;
            if (frag.orphan == true)
                co_orphan = ColorP::FRAG_ORPHAN;

            for (const auto &path : frag.paths) {
                for (size_t i = 0; i < path.size(); i++) {
                    auto &c0 = path[i];
                    auto &c1 = path[(i + 1) % path.size()];
                    draw_line(Coordf(c0.X, c0.Y), Coordf(c1.X, c1.Y), co_orphan, poly.layer, !transform_is_identity);
                }
            }
        }
        if (plane->fragments.size() == 0) { // empty, draw poly outline
            for (size_t i = 0; i < poly.vertices.size(); i++) {
                draw_line(poly.vertices[i].position, poly.vertices[(i + 1) % poly.vertices.size()].position, co,
                          poly.layer, !transform_is_identity);
            }
        }
        end_group();
        object_ref_pop();
    }
    else { // normal polygon
        const bool is_keepout = dynamic_cast<Keepout *>(poly.usage.ptr);
        begin_group(poly.layer);
        if (poly.is_rect() && !is_keepout) {
            const Coordf p0 = (poly.get_vertex(0).position + poly.get_vertex(1).position) / 2;
            const Coordf p1 = (poly.get_vertex(2).position + poly.get_vertex(3).position) / 2;
            const float width = (poly.get_vertex(0).position - poly.get_vertex(1).position).magd();
            add_triangle(poly.layer, transform.transform(p0), transform.transform(p1), Coordf(width, 1), co,
                         TriangleInfo::FLAG_BUTT);
        }
        else {
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
            po.SetOrientation(TPPL_ORIENTATION_CCW);
            part.Triangulate_EC(&po, &outpolys);
            if (is_keepout) {
                assert(triangle_type_current == TriangleInfo::Type::NONE);
                triangle_type_current = TriangleInfo::Type::KEEPOUT_FILL;
            }
            for (auto &tri : outpolys) {
                assert(tri.GetNumPoints() == 3);
                Coordf p0 = transform.transform(coordf_from_pt(tri[0]));
                Coordf p1 = transform.transform(coordf_from_pt(tri[1]));
                Coordf p2 = transform.transform(coordf_from_pt(tri[2]));
                add_triangle(poly.layer, p0, p1, p2, co);
            }
            if (is_keepout) {
                triangle_type_current = TriangleInfo::Type::NONE;
            }
            for (size_t i = 0; i < poly.vertices.size(); i++) {
                draw_line(poly.vertices[i].position, poly.vertices[(i + 1) % poly.vertices.size()].position, co,
                          poly.layer);
            }
        }
        end_group();
    }

    if (interactive) {
        selectables.group_begin();
        for (int i = 0; i < static_cast<int>(ipoly.vertices.size()); i++) {
            const auto &v = ipoly.get_vertex(i);
            const auto &v_last = ipoly.get_vertex(i - 1);
            int i_last = i - 1;
            if (i_last < 0)
                i_last += ipoly.vertices.size();
            if (v_last.type != Polygon::Vertex::Type::ARC) {
                auto center = (v.position + v_last.position) / 2;
                selectables.append_line(ipoly.uuid, ObjectType::POLYGON_EDGE, v_last.position, v.position, 0, i_last,
                                        ipoly.layer);
                targets.emplace_back(ipoly.uuid, ObjectType::POLYGON_EDGE, center, i_last, ipoly.layer);
            }
            else {
                const Coordf b = v.position;
                const Coordf a = v_last.position;
                const Coordf c = project_onto_perp_bisector(a, b, v_last.arc_center);
                const auto r = (a - c).mag();
                float a0 = (a - c).angle();
                float a1 = (b - c).angle();
                if (v_last.arc_reverse)
                    std::swap(a0, a1);
                selectables.append_arc(ipoly.uuid, ObjectType::POLYGON_EDGE, c, r, r, a0, a1, i_last, ipoly.layer);
            }

            selectables.append(ipoly.uuid, ObjectType::POLYGON_VERTEX, v.position, i, ipoly.layer);
            targets.emplace_back(ipoly.uuid, ObjectType::POLYGON_VERTEX, v.position, i, ipoly.layer);
            if (v.type == Polygon::Vertex::Type::ARC) {
                selectables.append(ipoly.uuid, ObjectType::POLYGON_ARC_CENTER, v.arc_center, i, ipoly.layer);
                targets.emplace_back(ipoly.uuid, ObjectType::POLYGON_ARC_CENTER, v.arc_center, i, ipoly.layer);
            }
        }
        selectables.group_end();
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
        auto r = shape.params.at(0) / 2;
        transform_save();
        transform.accumulate(shape.placement);
        add_triangle(shape.layer, transform.shift, Coordf(r, NAN), Coordf(NAN, NAN), ColorP::FROM_LAYER);
        transform_restore();
    }
    else if (shape.form == Shape::Form::RECTANGLE) {
        transform_save();
        transform.accumulate(shape.placement);
        auto w = shape.params.at(0);
        auto h = shape.params.at(1);
        add_triangle(shape.layer, transform.transform(Coordf(-w / 2, 0)), transform.transform(Coordf(w / 2, 0)),
                     Coordf(h, 0), ColorP::FROM_LAYER, TriangleInfo::FLAG_BUTT);
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
    const int64_t r = hole.diameter / 2;
    const int64_t l = std::max((int64_t)hole.length / 2 - r, (int64_t)0);
    const int layer = get_overlay_layer(hole.span);
    if (hole.shape == Hole::Shape::ROUND) {
        draw_circle(Coordf(), r, co, layer);
        if (hole.plated) {
            draw_circle(Coordf(), r * 0.9, co, layer);
        }
        float x = hole.diameter / 2 / M_SQRT2;
        draw_line(Coordi(-x, -x), Coordi(x, x), co, layer);
        draw_line(Coordi(x, -x), Coordi(-x, x), co, layer);
        if (interactive)
            selectables.append(hole.uuid, ObjectType::HOLE, Coordi(), Coordi(-r, -r), Coordi(r, r), 0, hole.span);
    }
    else if (hole.shape == Hole::Shape::SLOT) {
        draw_circle(Coordi(-l, 0), r, co, layer);
        draw_circle(Coordi(l, 0), r, co, layer);
        draw_line(Coordi(-l, -r), Coordi(l, -r), co, layer);
        draw_line(Coordi(-l, r), Coordi(l, r), co, layer);
        if (interactive)
            selectables.append(hole.uuid, ObjectType::HOLE, Coordi(), Coordi(-l - r, -r), Coordi(l + r, +r), 0,
                               hole.span);
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
    triangle_type_current = TriangleInfo::Type::PAD;
    render(pad.padstack, false);
    triangle_type_current = TriangleInfo::Type::NONE;
    img_patch_type(PatchType::OTHER);
    img_net(nullptr);
    transform_restore();
}

void Canvas::render(const Symbol &sym, SymbolMode mode, bool smashed, ColorP co)
{
    const bool on_sheet = mode == SymbolMode::SHEET;
    if (!on_sheet) {
        for (const auto &it : sym.junctions) {
            auto &junc = it.second;
            selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, 10000, mode == SymbolMode::EDIT);
            targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position));
        }
    }

    for (const auto &it : sym.lines) {
        render(it.second, !on_sheet, co);
    }

    if (object_refs_current.size() && object_refs_current.back().type == ObjectType::SCHEMATIC_SYMBOL) {
        auto sym_uuid = object_refs_current.back().uuid;
        for (const auto &it : sym.pins) {
            object_ref_push(ObjectType::SYMBOL_PIN, it.second.uuid, sym_uuid);
            render(it.second, mode, co);
            object_ref_pop();
        }
    }
    else {
        for (const auto &it : sym.pins) {
            render(it.second, mode, co);
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
        render(it.second);
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
    for (const auto &it : sheet.block_symbols) {
        render(it.second);
    }
    for (const auto &it : sheet.net_ties) {
        render(it.second);
    }
}

void Canvas::render(const Padstack &padstack, bool interactive)
{
    img_padstack(padstack);
    img_set_padstack(true);
    for (const auto &it : padstack.polygons) {
        render(it.second, interactive);
    }
    for (const auto &it : padstack.shapes) {
        render(it.second, interactive);
    }
    img_set_padstack(false);
    for (const auto &it : padstack.holes) {
        render(it.second, interactive);
    }
}

void Canvas::render_pad_overlay(const Pad &pad, bool interactive)
{
    if (img_mode)
        return;
    if (pad.padstack.type == Padstack::Type::MECHANICAL && !interactive)
        return;
    auto bb = pad.padstack.get_bbox(true);  // only copper
    if (bb.second - bb.first == Coordi()) { // empty bbox
        bb = pad.padstack.get_bbox(false);  // everything
    }
    if (bb.second - bb.first == Coordi()) { // still empty??
        return;
    }
    transform_save();
    transform.accumulate(pad.placement);
    auto a = bb.first;
    auto b = bb.second;
    transform.accumulate(Placement((a + b) / 2));

    auto pad_width = abs(b.x - a.x);
    auto pad_height = abs(b.y - a.y);

    int text_layer = 10000;
    switch (pad.padstack.type) {
    case Padstack::Type::TOP:
        text_layer = get_overlay_layer(BoardLayers::TOP_COPPER, true);
        break;

    case Padstack::Type::BOTTOM:
        text_layer = get_overlay_layer(BoardLayers::BOTTOM_COPPER, true);
        break;

    default:
        text_layer = get_overlay_layer({BoardLayers::TOP_COPPER, BoardLayers::BOTTOM_COPPER}, true);
    }

    Placement tr;
    tr.set_angle_rad(get_view_angle());
    if (get_flip_view())
        tr.invert_angle();
    {
        Placement tr2 = transform;
        if (tr2.mirror)
            tr2.invert_angle();
        tr2.mirror = false;
        tr.accumulate(tr2);
    }
    if (get_flip_view()) {
        tr.shift.x *= -1;
        tr.invert_angle();
    }

    set_lod_size(std::min(pad_height, pad_width));

    if (pad.secondary_text.size()) {
        draw_bitmap_text_box(tr, pad_width, pad_height, pad.name, ColorP::TEXT_OVERLAY, text_layer, TextBoxMode::UPPER);
        draw_bitmap_text_box(tr, pad_width, pad_height, pad.secondary_text, ColorP::TEXT_OVERLAY, text_layer,
                             TextBoxMode::LOWER);
    }
    else {
        draw_bitmap_text_box(tr, pad_width, pad_height, pad.name, ColorP::TEXT_OVERLAY, text_layer, TextBoxMode::FULL);
    }

    set_lod_size(-1);
    transform_restore();
}

static LayerRange get_layer_for_padstack_type(Padstack::Type type)
{
    if (type == Padstack::Type::TOP) {
        return BoardLayers::TOP_COPPER;
    }
    else if (type == Padstack::Type::BOTTOM) {
        return BoardLayers::BOTTOM_COPPER;
    }
    else {
        return LayerRange(BoardLayers::BOTTOM_COPPER, BoardLayers::TOP_COPPER);
    }
}

void Canvas::render(const Package &pkg, bool interactive, bool smashed, bool omit_silkscreen, bool omit_outline,
                    bool on_panel)
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
        // rendering as a board package, not on a panel
        auto pkg_uuid = object_refs_current.back().uuid;
        for (const auto &it : pkg.pads) {
            object_ref_push(ObjectType::PAD, it.second.uuid, pkg_uuid);
            render_pad_overlay(it.second, interactive);
            render(it.second);
            object_ref_pop();
        }
    }
    else if (on_panel) {
        for (const auto &it : pkg.pads) {
            render_pad_overlay(it.second, interactive);
            render(it.second);
        }
    }
    else {
        for (const auto &it : pkg.pads) {
            object_ref_push(ObjectType::PAD, it.second.uuid);
            render_pad_overlay(it.second, interactive);
            render(it.second);
            object_ref_pop();
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
        for (const auto &[pad_uuid, pad] : pkg.pads) {
            transform_save();
            transform.accumulate(pad.placement);
            const auto bb = pad.padstack.get_bbox();
            const auto layer = get_layer_for_padstack_type(pad.padstack.type);
            selectables.append(pad.uuid, ObjectType::PAD, {0, 0}, bb.first, bb.second, 0, layer);
            transform_restore();
            targets.emplace_back(pad.uuid, ObjectType::PAD, pad.placement.shift, 0, layer);
            if (add_pad_bbox_targets) {
                const auto bb_cu = pad.padstack.get_bbox(true);
                size_t i = 1;
                for (const auto x : {bb_cu.first.x, bb_cu.second.x}) {
                    for (const auto y : {bb_cu.first.y, bb_cu.second.y}) {
                        targets.emplace_back(pad.uuid, ObjectType::PAD, pad.placement.transform(Coordi(x, y)), i++,
                                             layer);
                    }
                }
            }
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
                           pkg.flip ? BoardLayers::BOTTOM_COPPER : BoardLayers::TOP_COPPER);
        targets.emplace_back(pkg.uuid, ObjectType::BOARD_PACKAGE, pkg.placement.shift);

        for (const auto &[pad_uuid, pad] : pkg.package.pads) {
            const auto layer = get_layer_for_padstack_type(pad.padstack.type);
            targets.emplace_back(UUIDPath<2>(pkg.uuid, pad.uuid), ObjectType::PAD,
                                 transform.transform(pad.placement.shift), 0, layer);
        }
    }
    if (interactive)
        object_ref_push(ObjectType::BOARD_PACKAGE, pkg.uuid);

    render(pkg.package, false, pkg.smashed, pkg.omit_silkscreen, pkg.omit_outline, !interactive);

    if (interactive)
        object_ref_pop();

    transform.reset();
    transform_restore();
}

static std::string layer_to_string(int l)
{
    switch (l) {
    case BoardLayers::TOP_COPPER:
        return "T";
    case BoardLayers::BOTTOM_COPPER:
        return "B";
    default:
        if (l < BoardLayers::TOP_COPPER && l > BoardLayers::BOTTOM_COPPER) {
            return "I" + std::to_string(-l);
        }
    }
    return "?";
}

static std::string span_to_string(const LayerRange &r)
{
    return layer_to_string(r.end()) + ":" + layer_to_string(r.start());
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
        selectables.append(via.uuid, ObjectType::VIA, {0, 0}, bb.first, bb.second, 0, via.junction->layer);
    img_net(via.junction->net);
    img_patch_type(PatchType::VIA);
    if (interactive)
        object_ref_push(ObjectType::VIA, via.uuid);
    render(via.padstack, false);
    if (via.locked) {
        auto ol = get_overlay_layer(via.span.end());
        draw_lock({0, 0}, 0.7 * std::min(std::abs(bb.second.x - bb.first.x), std::abs(bb.second.y - bb.first.y)),
                  ColorP::TEXT_OVERLAY, ol, true);
    }
    const bool show_net = show_text_in_vias && via.junction->net && via.junction->net->name.size();
    const bool show_span =
            show_via_span == ShowViaSpan::ALL
            || (via.span != BoardLayers::layer_range_through && show_via_span == ShowViaSpan::BLIND_BURIED);
    if (interactive && (show_net || show_span)) {
        auto size = (bb.second.x - bb.first.x) * 1.2;
        set_lod_size(size);
        Placement p;
        p.set_angle_rad(get_view_angle());
        if (get_flip_view())
            p.invert_angle();
        p.accumulate(Placement(via.junction->position));
        if (get_flip_view()) {
            p.shift.x *= -1;
        }
        p.set_angle(0);
        const auto ol = get_overlay_layer(via.span, true);
        if (show_net && show_span) {
            draw_bitmap_text_box(p, size, size, via.junction->net->name, ColorP::TEXT_OVERLAY, ol, TextBoxMode::UPPER);
            draw_bitmap_text_box(p, size, size, span_to_string(via.span), ColorP::TEXT_OVERLAY, ol, TextBoxMode::LOWER);
        }
        else if (show_net && !show_span) {
            draw_bitmap_text_box(p, size, size, via.junction->net->name, ColorP::TEXT_OVERLAY, ol, TextBoxMode::FULL);
        }
        else if (!show_net && show_span) {
            draw_bitmap_text_box(p, size, size, span_to_string(via.span), ColorP::TEXT_OVERLAY, ol, TextBoxMode::FULL);
        }
        set_lod_size(-1);
    }


    if (interactive)
        object_ref_pop();
    img_net(nullptr);
    img_patch_type(PatchType::OTHER);
    transform_restore();
}

void Canvas::render(const BoardHole &hole, bool interactive)
{
    transform_save();
    transform.accumulate(hole.placement);
    auto bb = hole.padstack.get_bbox();
    if (interactive) {
        const auto layers = LayerRange(BoardLayers::TOP_COPPER, BoardLayers::BOTTOM_COPPER);
        selectables.append(hole.uuid, ObjectType::BOARD_HOLE, {0, 0}, bb.first, bb.second, 0, layers);
        targets.emplace_back(hole.uuid, ObjectType::BOARD_HOLE, transform.transform(Coordi()), 0, layers);
    }
    img_net(hole.net);
    if (hole.padstack.type == Padstack::Type::HOLE)
        img_patch_type(PatchType::HOLE_PTH);
    else
        img_patch_type(PatchType::HOLE_NPTH);
    if (interactive)
        object_ref_push(ObjectType::BOARD_HOLE, hole.uuid);
    render(hole.padstack, false);
    if (interactive)
        object_ref_pop();
    img_net(nullptr);
    img_patch_type(PatchType::OTHER);
    transform_restore();
}

void Canvas::render(const class Dimension &dim)
{
    if (img_mode)
        return;

    Coordd p0 = dim.p0;
    Coordd p1 = dim.p1;

    if (dim.mode == Dimension::Mode::HORIZONTAL) {
        p1 = Coordd(p1.x, p0.y);
    }
    else if (dim.mode == Dimension::Mode::VERTICAL) {
        p1 = Coordd(p0.x, p1.y);
    }

    const Coordd v = p1 - p0;
    const auto vn = v.normalize();
    Coordd w = Coordd(-v.y, v.x);
    if (dim.mode == Dimension::Mode::HORIZONTAL) {
        w.y = std::abs(w.y);
    }
    else if (dim.mode == Dimension::Mode::VERTICAL) {
        w.x = std::abs(w.x);
    }
    const auto wn = w.normalize();

    ColorP co = ColorP::DIMENSION;

    selectables.group_begin();
    if (v.mag_sq()) {

        auto q0 = p0 + wn * dim.label_distance;
        auto q1 = p1 + wn * dim.label_distance;

        draw_line(dim.p0, p0 + wn * (dim.label_distance + sgn(dim.label_distance) * (int64_t)dim.label_size / 2), co,
                  10000, true, 0);
        draw_line(dim.p1, p1 + wn * (dim.label_distance + sgn(dim.label_distance) * (int64_t)dim.label_size / 2), co,
                  10000, true, 0);

        auto length = v.mag();
        auto s = dim_to_string(length, false);

        TextRenderer::Options opts;
        opts.draw = false;
        const auto text_bb = draw_text({0, 0}, dim.label_size, s, 0, TextOrigin::CENTER, co, 10000, opts);
        auto text_width = std::abs(text_bb.second.x - text_bb.first.x);

        Coordf text_pos = (q0 + v / 2) - (vn * text_width / 2);
        auto angle = v.angle();

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

        const int angle_i = angle_from_rad(angle);
        TextRenderer::Options opts2;
        opts2.flip = get_flip_view();
        const auto real_text_bb =
                draw_text(Coordi(text_pos.x, text_pos.y), dim.label_size, s,
                          get_flip_view() ? (32768 - angle_i) : angle_i, TextOrigin::CENTER, co, 10000, opts2);

        selectables.append(dim.uuid, ObjectType::DIMENSION, q0, real_text_bb.first, real_text_bb.second, 2);
        targets.emplace_back(dim.uuid, ObjectType::DIMENSION, Coordi(q0.x, q0.y), 2);
    }

    selectables.append(dim.uuid, ObjectType::DIMENSION, dim.p0, 0);
    selectables.append(dim.uuid, ObjectType::DIMENSION, dim.p1, 1);
    selectables.group_end();

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
    for (const auto &it : brd.decals) {
        render(it.second);
    }
    for (const auto &it : brd.net_ties) {
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

void Canvas::render(const Decal &dec, bool interactive)
{
    if (interactive) {
        for (const auto &it : dec.junctions) {
            auto &junc = it.second;
            selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, 10000, true);
            targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position));
        }
    }
    for (const auto &it : dec.lines) {
        render(it.second, interactive);
    }
    for (const auto &it : dec.arcs) {
        render(it.second, interactive);
    }
    for (const auto &it : dec.polygons) {
        render(it.second, interactive);
    }
    for (const auto &it : dec.texts) {
        render(it.second, interactive);
    }
}

void Canvas::render(const Picture &pic, bool interactive)
{
    if (!pic.data && !img_mode) {
        draw_error(pic.placement.shift, 2e5, "Image " + (std::string)pic.data_uuid + " not found");
        selectables.append_angled(pic.uuid, ObjectType::PICTURE, pic.placement.shift, pic.placement.shift,
                                  Coordf(1_mm, 1_mm), 0);
        return;
    }
    if (img_mode)
        return;
    pictures.emplace_back();
    auto &x = pictures.back();
    auto tr = transform;
    tr.accumulate(pic.placement);
    x.angle = tr.get_angle_rad();
    x.x = tr.shift.x;
    x.y = tr.shift.y;
    x.px_size = pic.px_size;
    x.data = pic.data;
    x.on_top = pic.on_top;
    x.opacity = pic.opacity;
    if (interactive) {
        float w = pic.data->width * pic.px_size;
        float h = pic.data->height * pic.px_size;
        selectables.append_angled(pic.uuid, ObjectType::PICTURE, pic.placement.shift, pic.placement.shift, {w, h},
                                  pic.placement.get_angle_rad());
    }
}

void Canvas::render(const BoardDecal &decal)
{
    transform_save();
    transform.accumulate(decal.placement);
    auto bb = decal.get_decal().get_bbox();
    if (decal.get_flip()) {
        transform.invert_angle();
    }
    selectables.append(decal.uuid, ObjectType::BOARD_DECAL, {0, 0}, bb.first, bb.second, 0, decal.get_layers());
    render(decal.get_decal(), false);
    transform_restore();
}

void Canvas::render(const BlockSymbol &sym, bool on_sheet)
{
    if (!on_sheet) {
        for (const auto &it : sym.junctions) {
            auto &junc = it.second;
            selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, 10000, true);
            targets.emplace_back(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position));
        }
    }
    for (const auto &it : sym.lines) {
        render(it.second, !on_sheet);
    }

    if (object_refs_current.size() && object_refs_current.back().type == ObjectType::SCHEMATIC_BLOCK_SYMBOL) {
        auto sym_uuid = object_refs_current.back().uuid;
        for (const auto &it : sym.ports) {
            object_ref_push(ObjectType::BLOCK_SYMBOL_PORT, it.second.uuid, sym_uuid);
            render(it.second, !on_sheet);
            object_ref_pop();
        }
    }
    else {
        for (const auto &it : sym.ports) {
            render(it.second, !on_sheet);
        }
    }


    for (const auto &it : sym.arcs) {
        render(it.second, !on_sheet);
    }
    for (const auto &it : sym.texts) {
        render(it.second, !on_sheet);
    }
    for (const auto &it : sym.pictures) {
        render(it.second, !on_sheet);
    }
}

void Canvas::draw_direction(Pin::Direction dir, ColorP color)
{
    auto dl = [this, color](float ax, float ay, float bx, float by) {
        draw_line(Coordf(ax * 1_mm, ay * 1_mm), Coordf(bx * 1_mm, by * 1_mm), color, 0, true, 0);
    };
    switch (dir) {
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

void Canvas::render(const class BlockSymbolPort &port, bool interactive)
{
    Coordi p0 = transform.transform(port.position);
    Coordi p1 = p0;

    Coordi p_name = p0;
    Coordi p_nc = p0;

    Orientation port_orientation = port.get_orientation_for_placement(transform);

    Orientation name_orientation = Orientation::LEFT;
    int64_t text_shift = 0.5_mm;
    int64_t text_shift_name = text_shift;
    int64_t nc_shift = 0.25_mm;
    int64_t length = port.length;
    const auto nc_orientation = port_orientation;

    switch (port_orientation) {
    case Orientation::LEFT:
        p1.x += length;
        p_name.x += port.length + text_shift_name;
        p_nc.x -= nc_shift;
        name_orientation = Orientation::RIGHT;
        break;

    case Orientation::RIGHT:
        p1.x -= length;
        p_name.x -= port.length + text_shift_name;
        p_nc.x += nc_shift;
        name_orientation = Orientation::LEFT;
        break;

    case Orientation::UP:
        p1.y -= length;
        p_name.y -= port.length + text_shift_name;
        p_nc.y += nc_shift;
        name_orientation = Orientation::DOWN;
        break;

    case Orientation::DOWN:
        p1.y += length;
        p_name.y += port.length + text_shift_name;
        p_nc.y -= nc_shift;
        name_orientation = Orientation::UP;
        break;
    }
    ColorP c_main = ColorP::FROM_LAYER;
    ColorP c_name = ColorP::PIN;
    ColorP c_port = ColorP::PIN;

    img_auto_line = img_mode;

    const bool draw_in_line = port.name_orientation == PinNameOrientation::IN_LINE
                              || (port.name_orientation == PinNameOrientation::HORIZONTAL
                                  && (port_orientation == Orientation::LEFT || port_orientation == Orientation::RIGHT));

    if (draw_in_line) {
        draw_text(p_name, 1.5_mm, port.name, orientation_to_angle(name_orientation), TextOrigin::CENTER, c_name, 0, {});
    }
    else { // draw perp
        Placement tr;
        tr.set_angle(orientation_to_angle(port_orientation));
        auto shift = tr.transform(Coordi(-1_mm, 0));
        TextRenderer::Options opts;
        opts.center = true;
        draw_text(p_name + shift, 1.5_mm, port.name, orientation_to_angle(name_orientation) + 16384, TextOrigin::CENTER,
                  c_name, 0, opts);
    }


    transform_save();
    transform.accumulate(port.position);
    transform.set_angle(0);
    transform.mirror = false;
    switch (port_orientation) {
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

    draw_direction(port.direction, c_port);
    transform_restore();

    switch (port.connector_style) {
    case BlockSymbolPort::ConnectorStyle::BOX:
        draw_box(p0, 0.25_mm, c_main, 0, false);
        break;

    case BlockSymbolPort::ConnectorStyle::NC:
        draw_cross(p0, 0.25_mm, c_main, 0, false);
        draw_text(p_nc, 1.5_mm, "NC", orientation_to_angle(nc_orientation), TextOrigin::CENTER, c_port, 0, {});
        break;

    case BlockSymbolPort::ConnectorStyle::NONE:
        break;
    }
    /* if (port.connected_net_lines.size() > 1) {
         draw_line(p0, p0 + Coordi(0, 10), c_main, 0, false, 0.75_mm);
     }*/
    draw_line(p0, p1, c_main, 0, false);
    if (interactive)
        selectables.append_line(port.uuid, ObjectType::BLOCK_SYMBOL_PORT, p0, p1, 0);
    img_auto_line = false;
}

void Canvas::render(const SchematicBlockSymbol &sym)
{
    transform = sym.placement;
    object_ref_push(ObjectType::SCHEMATIC_BLOCK_SYMBOL, sym.uuid);
    render(sym.symbol, true);
    object_ref_pop();
    for (const auto &it : sym.symbol.ports) {
        targets.emplace_back(UUIDPath<2>(sym.uuid, it.second.uuid), ObjectType::BLOCK_SYMBOL_PORT,
                             transform.transform(it.second.position));
    }
    auto bb = sym.symbol.get_bbox();
    selectables.append(sym.uuid, ObjectType::SCHEMATIC_BLOCK_SYMBOL, {0, 0}, bb.first, bb.second);
    transform.reset();
}


} // namespace horizon
