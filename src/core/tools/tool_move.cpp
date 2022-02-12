#include "tool_move.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "document/idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "document/idocument_symbol.hpp"
#include "document/idocument_block_symbol.hpp"
#include "block_symbol/block_symbol.hpp"
#include "pool/symbol.hpp"
#include "imp/imp_interface.hpp"
#include "util/accumulator.hpp"
#include "util/util.hpp"
#include "util/geom_util.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolMove::ToolMove(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolHelperMove(c, tid), ToolHelperMerge(c, tid)
{
}

ToolMove::Axis operator|(ToolMove::Axis a, ToolMove::Axis b)
{
    return static_cast<ToolMove::Axis>(static_cast<int>(a) | static_cast<int>(b));
}

ToolMove::Axis &operator|=(ToolMove::Axis &lhs, const ToolMove::Axis &rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

ToolMove::Axis operator&(ToolMove::Axis a, ToolMove::Axis b)
{
    return static_cast<ToolMove::Axis>(static_cast<int>(a) & static_cast<int>(b));
}

void ToolMove::add_extra_junction(const UUID &uu, Axis ax)
{
    if (ax == Axis::NONE)
        return;
    if (extra_junctions.count(uu))
        extra_junctions.at(uu) |= ax;
    else
        extra_junctions.emplace(uu, ax);
}

void ToolMove::expand_selection()
{
    std::set<SelectableRef> pkgs_fixed;
    std::set<SelectableRef> new_sel;
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::LINE: {
            Line *line = doc.r->get_line(it.uuid);
            new_sel.emplace(line->from.uuid, ObjectType::JUNCTION);
            new_sel.emplace(line->to.uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::POLYGON_EDGE: {
            Polygon *poly = doc.r->get_polygon(it.uuid);
            auto vs = poly->get_vertices_for_edge(it.vertex);
            new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.first);
            new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.second);
        } break;

        case ObjectType::NET_LABEL: {
            auto &la = doc.c->get_sheet()->net_labels.at(it.uuid);
            new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::BUS_LABEL: {
            auto &la = doc.c->get_sheet()->bus_labels.at(it.uuid);
            new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::POWER_SYMBOL: {
            auto &ps = doc.c->get_sheet()->power_symbols.at(it.uuid);
            new_sel.emplace(ps.junction->uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::BUS_RIPPER: {
            auto &rip = doc.c->get_sheet()->bus_rippers.at(it.uuid);
            new_sel.emplace(rip.junction->uuid, ObjectType::JUNCTION);
        } break;

        case ObjectType::LINE_NET: {
            auto line = &doc.c->get_sheet()->net_lines.at(it.uuid);
            for (auto &it_ft : {line->from, line->to}) {
                if (it_ft.is_junc()) {
                    new_sel.emplace(it_ft.junc.uuid, ObjectType::JUNCTION);
                }
            }
        } break;
        case ObjectType::TRACK: {
            auto track = &doc.b->get_board()->tracks.at(it.uuid);
            for (auto &it_ft : {track->from, track->to}) {
                if (it_ft.is_junc()) {
                    new_sel.emplace(it_ft.junc.uuid, ObjectType::JUNCTION);
                }
            }
        } break;
        case ObjectType::VIA: {
            auto via = &doc.b->get_board()->vias.at(it.uuid);
            new_sel.emplace(via->junction->uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::POLYGON: {
            auto poly = doc.r->get_polygon(it.uuid);
            int i = 0;
            for (const auto &itv : poly->vertices) {
                (void)sizeof itv;
                new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, i);
                i++;
            }
        } break;

        case ObjectType::ARC: {
            Arc *arc = doc.r->get_arc(it.uuid);
            new_sel.emplace(arc->from.uuid, ObjectType::JUNCTION);
            new_sel.emplace(arc->to.uuid, ObjectType::JUNCTION);
            new_sel.emplace(arc->center.uuid, ObjectType::JUNCTION);
        } break;

        case ObjectType::SCHEMATIC_SYMBOL: {
            auto &sym = doc.c->get_sheet()->symbols.at(it.uuid);
            for (const auto &itt : sym.texts) {
                new_sel.emplace(itt->uuid, ObjectType::TEXT);
            }
            for (const auto &[pin_uu, pin] : sym.symbol.pins) {
                for (const auto &[line_uu, line] : doc.c->get_sheet()->net_lines) {
                    if (line.is_connected_to_symbol(sym.uuid, pin_uu)) {
                        for (const auto &it_ft : {line.from, line.to}) {
                            if (it_ft.is_junc()) {
                                const auto &ju = *it_ft.junc;
                                const auto pin_pos = sym.placement.transform(pin.position);
                                Axis ax = Axis::NONE;
                                if (pin_pos.x == ju.position.x)
                                    ax = Axis::X;
                                else if (pin_pos.y == ju.position.y)
                                    ax = Axis::Y;

                                if (ju.connected_net_lines.size() == 1) { // dangling end
                                    ax = Axis::X | Axis::Y;
                                }
                                add_extra_junction(ju.uuid, ax);
                            }
                        }
                    }
                }
            }
        } break;


        case ObjectType::SCHEMATIC_BLOCK_SYMBOL: {
            auto &sym = doc.c->get_sheet()->block_symbols.at(it.uuid);
            for (const auto &[pin_uu, port] : sym.symbol.ports) {
                for (const auto &[line_uu, line] : doc.c->get_sheet()->net_lines) {
                    if (line.is_connected_to_block_symbol(sym.uuid, pin_uu)) {
                        for (const auto &it_ft : {line.from, line.to}) {
                            if (it_ft.is_junc()) {
                                const auto &ju = *it_ft.junc;
                                const auto port_pos = sym.placement.transform(port.position);
                                Axis ax = Axis::NONE;
                                if (port_pos.x == ju.position.x)
                                    ax = Axis::X;
                                else if (port_pos.y == ju.position.y)
                                    ax = Axis::Y;

                                if (ju.connected_net_lines.size() == 1) { // dangling end
                                    ax = Axis::X | Axis::Y;
                                }
                                add_extra_junction(ju.uuid, ax);
                            }
                        }
                    }
                }
            }
        } break;

        case ObjectType::BOARD_PACKAGE: {
            BoardPackage *pkg = &doc.b->get_board()->packages.at(it.uuid);
            if (pkg->fixed) {
                pkgs_fixed.insert(it);
            }
            else {
                for (const auto &itt : pkg->texts) {
                    new_sel.emplace(itt->uuid, ObjectType::TEXT);
                }
            }
        } break;
        default:;
        }
    }
    selection.insert(new_sel.begin(), new_sel.end());
    map_erase_if(extra_junctions,
                 [this](const auto &x) { return selection.count(SelectableRef(x.first, ObjectType::JUNCTION)); });
    if (pkgs_fixed.size() && imp)
        imp->tool_bar_flash("can't move fixed package");
    for (auto it = selection.begin(); it != selection.end();) {
        if (pkgs_fixed.count(*it))
            it = selection.erase(it);
        else
            ++it;
    }
}

Coordi ToolMove::get_selection_center()
{
    Accumulator<Coordi> accu;
    std::set<SelectableRef> items_ignore;
    for (const auto &it : selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            const auto &pkg = doc.b->get_board()->packages.at(it.uuid);
            for (auto &it_txt : pkg.texts) {
                items_ignore.emplace(it_txt->uuid, ObjectType::TEXT);
            }
        }
        else if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            const auto &sym = doc.c->get_sheet()->symbols.at(it.uuid);
            for (auto &it_txt : sym.texts) {
                items_ignore.emplace(it_txt->uuid, ObjectType::TEXT);
            }
        }
    }
    for (const auto &it : selection) {
        if (items_ignore.count(it))
            continue;
        switch (it.type) {
        case ObjectType::JUNCTION:
            accu.accumulate(doc.r->get_junction(it.uuid)->position);
            break;
        case ObjectType::HOLE:
            accu.accumulate(doc.r->get_hole(it.uuid)->placement.shift);
            break;
        case ObjectType::BOARD_HOLE:
            accu.accumulate(doc.b->get_board()->holes.at(it.uuid).placement.shift);
            break;
        case ObjectType::SYMBOL_PIN:
            accu.accumulate(doc.y->get_symbol_pin(it.uuid).position);
            break;
        case ObjectType::BLOCK_SYMBOL_PORT:
            accu.accumulate(doc.o->get_block_symbol().ports.at(it.uuid).position);
            break;
        case ObjectType::SCHEMATIC_SYMBOL:
            accu.accumulate(doc.c->get_sheet()->symbols.at(it.uuid).placement.shift);
            break;
        case ObjectType::SCHEMATIC_BLOCK_SYMBOL:
            accu.accumulate(doc.c->get_sheet()->block_symbols.at(it.uuid).placement.shift);
            break;
        case ObjectType::BOARD_PACKAGE:
            accu.accumulate(doc.b->get_board()->packages.at(it.uuid).placement.shift);
            break;
        case ObjectType::PAD:
            accu.accumulate(doc.k->get_package().pads.at(it.uuid).placement.shift);
            break;
        case ObjectType::TEXT:
            accu.accumulate(doc.r->get_text(it.uuid)->placement.shift);
            break;
        case ObjectType::POLYGON_VERTEX:
            accu.accumulate(doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).position);
            break;
        case ObjectType::DIMENSION:
            if (it.vertex < 2) {
                auto dim = doc.r->get_dimension(it.uuid);
                accu.accumulate(it.vertex == 0 ? dim->p0 : dim->p1);
            }
            break;
        case ObjectType::POLYGON_ARC_CENTER:
            accu.accumulate(doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_center);
            break;
        case ObjectType::SHAPE:
            accu.accumulate(doc.a->get_padstack().shapes.at(it.uuid).placement.shift);
            break;
        case ObjectType::BOARD_PANEL:
            accu.accumulate(doc.b->get_board()->board_panels.at(it.uuid).placement.shift);
            break;
        case ObjectType::PICTURE:
            accu.accumulate(doc.r->get_picture(it.uuid)->placement.shift);
            break;
        case ObjectType::BOARD_DECAL:
            accu.accumulate(doc.b->get_board()->decals.at(it.uuid).placement.shift);
            break;
        default:;
        }
    }
    if (doc.c || doc.y)
        return (accu.get() / 1.25_mm) * 1.25_mm;
    else
        return accu.get();
}

ToolResponse ToolMove::begin(const ToolArgs &args)
{
    std::cout << "tool move\n";
    move_init(args.coords);

    Coordi selection_center;
    if (tool_id == ToolID::ROTATE_CURSOR || tool_id == ToolID::MIRROR_CURSOR)
        selection_center = args.coords;
    else
        selection_center = get_selection_center();

    nets = nets_from_selection(selection);

    if (tool_id == ToolID::ROTATE || tool_id == ToolID::MIRROR_X || tool_id == ToolID::MIRROR_Y
        || tool_id == ToolID::ROTATE_CURSOR || tool_id == ToolID::MIRROR_CURSOR) {
        move_mirror_or_rotate(selection_center, tool_id == ToolID::ROTATE || tool_id == ToolID::ROTATE_CURSOR);
        if (tool_id == ToolID::MIRROR_Y) {
            move_mirror_or_rotate(selection_center, true);
            move_mirror_or_rotate(selection_center, true);
        }
        if (finish())
            return ToolResponse::commit();
        else
            return ToolResponse::revert();
    }
    if (tool_id == ToolID::MOVE_EXACTLY) {
        if (auto r = imp->dialogs.ask_datum_coord("Move exactly")) {
            move_do(*r);
            if (finish())
                return ToolResponse::commit();
            else
                return ToolResponse::revert();
        }
        else {
            return ToolResponse::end();
        }
    }

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::ROTATE, InToolActionID::MIRROR, "rotate/mirror"},
            {InToolActionID::ROTATE_CURSOR, InToolActionID::MIRROR_CURSOR, "rotate/mirror around cursor"},
            {InToolActionID::RESTRICT},
    });
    update_tip();

    for (const auto &it : selection) {
        if (it.type == ObjectType::POLYGON_VERTEX || it.type == ObjectType::POLYGON_EDGE) {
            auto poly = doc.r->get_polygon(it.uuid);
            if (auto plane = dynamic_cast<Plane *>(poly->usage.ptr)) {
                if (plane->fragments.size())
                    planes.insert(plane);
            }
        }
    }

    for (auto plane : planes) {
        plane->clear();
    }

    InToolActionID action = InToolActionID::NONE;
    switch (tool_id) {
    case ToolID::MOVE_KEY_FINE_UP:
        action = InToolActionID::MOVE_UP_FINE;
        break;
    case ToolID::MOVE_KEY_UP:
        action = InToolActionID::MOVE_UP;
        break;

    case ToolID::MOVE_KEY_FINE_DOWN:
        action = InToolActionID::MOVE_DOWN_FINE;
        break;

    case ToolID::MOVE_KEY_DOWN:
        action = InToolActionID::MOVE_DOWN;
        break;

    case ToolID::MOVE_KEY_FINE_LEFT:
        action = InToolActionID::MOVE_LEFT_FINE;
        break;

    case ToolID::MOVE_KEY_LEFT:
        action = InToolActionID::MOVE_LEFT;
        break;

    case ToolID::MOVE_KEY_FINE_RIGHT:
        action = InToolActionID::MOVE_RIGHT_FINE;
        break;

    case ToolID::MOVE_KEY_RIGHT:
        action = InToolActionID::MOVE_RIGHT;
        break;

    default:;
    }
    if (action != InToolActionID::NONE) {
        is_key = true;
        ToolArgs args2;
        args2.type = ToolEventType::ACTION;
        args2.action = action;
        update(args2);
    }
    if (tool_id == ToolID::MOVE_KEY)
        is_key = true;

    imp->tool_bar_set_tool_name("Move");


    return ToolResponse();
}

bool ToolMove::can_begin()
{
    expand_selection();
    return selection.size() > 0;
}

void ToolMove::update_tip()
{
    auto delta = get_delta();
    std::string s = coord_to_string(delta + key_delta, true) + " ";
    if (!is_key) {
        s += restrict_mode_to_string();
    }
    imp->tool_bar_set_tip(s);
}

void ToolMove::update_airwires()
{
    if (doc.b && nets.size()) {
        doc.b->get_board()->update_airwires(true, nets);
    }
}

void ToolMove::move_extra_junctions(const Coordi &delta)
{
    for (const auto &[uu, ax] : extra_junctions) {
        Coordi shift;
        if ((ax & Axis::X) != Axis::NONE) {
            shift.x = delta.x;
        }
        if ((ax & Axis::Y) != Axis::NONE) {
            shift.y = delta.y;
        }
        doc.r->get_junction(uu)->position += shift;
    }
}

void ToolMove::do_move(const Coordi &d)
{
    const auto delta = move_do_cursor(d);
    move_extra_junctions(delta);
    update_airwires();
    update_tip();
}

bool ToolMove::finish()
{
    std::set<UUID> junctions;
    for (const auto &[uu, ax] : extra_junctions)
        junctions.emplace(uu);
    merge_and_connect(junctions);
    if (doc.b) {
        imp->tool_bar_set_tip("updating planes…");
        auto brd = doc.b->get_board();
        for (auto plane : planes) {
            if (!imp->dialogs.update_plane(*brd, plane))
                return false;
        }
        brd->update_airwires(false, nets);
    }
    return true;
}

ToolResponse ToolMove::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (!is_key)
            do_move(args.coords);
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        if (any_of(args.action, {InToolActionID::LMB, InToolActionID::COMMIT})
            || (is_transient && args.action == InToolActionID::LMB_RELEASE)) {
            if (finish())
                return ToolResponse::commit();
            else
                return ToolResponse::revert();
        }
        else if (any_of(args.action, {InToolActionID::RMB, InToolActionID::CANCEL})) {
            return ToolResponse::revert();
        }
        else if (args.action == InToolActionID::RESTRICT) {
            cycle_restrict_mode();
            do_move(args.coords);
        }
        else if (any_of(args.action, {InToolActionID::ROTATE, InToolActionID::MIRROR})) {
            bool rotate = args.action == InToolActionID::ROTATE;
            const auto selection_center = get_selection_center();
            move_mirror_or_rotate(selection_center, rotate);
        }
        else if (any_of(args.action, {InToolActionID::ROTATE_CURSOR, InToolActionID::MIRROR_CURSOR})) {
            bool rotate = args.action == InToolActionID::ROTATE_CURSOR;
            move_mirror_or_rotate(args.coords, rotate);
        }
        else {
            const auto [dir, fine] = dir_from_action(args.action);
            if (dir.x || dir.y) {
                auto sp = imp->get_grid_spacing();
                auto shift = imp->transform_arrow_keys(dir * sp);
                if (fine)
                    shift = shift / 10;
                key_delta += shift;
                move_do(shift);
                move_extra_junctions(shift);
                update_airwires();
                update_tip();
            }
        }
    }
    return ToolResponse();
}
} // namespace horizon
