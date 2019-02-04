#include "tool_move.hpp"
#include "core_board.hpp"
#include "core_package.hpp"
#include "core_padstack.hpp"
#include "core_schematic.hpp"
#include "core_symbol.hpp"
#include "imp/imp_interface.hpp"
#include "util/accumulator.hpp"
#include "util/util.hpp"
#include <iostream>
#include <gtkmm.h>

namespace horizon {

ToolMove::ToolMove(Core *c, ToolID tid) : ToolBase(c, tid), ToolHelperMove(c, tid), ToolHelperMerge(c, tid)
{
}

void ToolMove::expand_selection()
{
    std::set<SelectableRef> new_sel;
    for (const auto &it : core.r->selection) {
        switch (it.type) {
        case ObjectType::LINE: {
            Line *line = core.r->get_line(it.uuid);
            new_sel.emplace(line->from.uuid, ObjectType::JUNCTION);
            new_sel.emplace(line->to.uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::POLYGON_EDGE: {
            Polygon *poly = core.r->get_polygon(it.uuid);
            auto vs = poly->get_vertices_for_edge(it.vertex);
            new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.first);
            new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.second);
        } break;

        case ObjectType::NET_LABEL: {
            auto &la = core.c->get_sheet()->net_labels.at(it.uuid);
            new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::BUS_LABEL: {
            auto &la = core.c->get_sheet()->bus_labels.at(it.uuid);
            new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::POWER_SYMBOL: {
            auto &ps = core.c->get_sheet()->power_symbols.at(it.uuid);
            new_sel.emplace(ps.junction->uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::BUS_RIPPER: {
            auto &rip = core.c->get_sheet()->bus_rippers.at(it.uuid);
            new_sel.emplace(rip.junction->uuid, ObjectType::JUNCTION);
        } break;

        case ObjectType::LINE_NET: {
            auto line = &core.c->get_sheet()->net_lines.at(it.uuid);
            for (auto &it_ft : {line->from, line->to}) {
                if (it_ft.is_junc()) {
                    new_sel.emplace(it_ft.junc.uuid, ObjectType::JUNCTION);
                }
            }
        } break;
        case ObjectType::TRACK: {
            auto track = &core.b->get_board()->tracks.at(it.uuid);
            for (auto &it_ft : {track->from, track->to}) {
                if (it_ft.is_junc()) {
                    new_sel.emplace(it_ft.junc.uuid, ObjectType::JUNCTION);
                }
            }
        } break;
        case ObjectType::VIA: {
            auto via = &core.b->get_board()->vias.at(it.uuid);
            new_sel.emplace(via->junction->uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::POLYGON: {
            auto poly = core.r->get_polygon(it.uuid);
            int i = 0;
            for (const auto &itv : poly->vertices) {
                (void)sizeof itv;
                new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, i);
                i++;
            }
        } break;

        case ObjectType::ARC: {
            Arc *arc = core.r->get_arc(it.uuid);
            new_sel.emplace(arc->from.uuid, ObjectType::JUNCTION);
            new_sel.emplace(arc->to.uuid, ObjectType::JUNCTION);
            new_sel.emplace(arc->center.uuid, ObjectType::JUNCTION);
        } break;

        case ObjectType::SCHEMATIC_SYMBOL: {
            auto sym = core.c->get_schematic_symbol(it.uuid);
            for (const auto &itt : sym->texts) {
                new_sel.emplace(itt->uuid, ObjectType::TEXT);
            }
        } break;
        case ObjectType::BOARD_PACKAGE: {
            BoardPackage *pkg = &core.b->get_board()->packages.at(it.uuid);
            for (const auto &itt : pkg->texts) {
                new_sel.emplace(itt->uuid, ObjectType::TEXT);
            }
        } break;
        default:;
        }
    }
    core.r->selection.insert(new_sel.begin(), new_sel.end());
}

void ToolMove::update_selection_center()
{
    Accumulator<Coordi> accu;
    for (const auto &it : core.r->selection) {
        switch (it.type) {
        case ObjectType::JUNCTION:
            accu.accumulate(core.r->get_junction(it.uuid)->position);
            break;
        case ObjectType::HOLE:
            accu.accumulate(core.r->get_hole(it.uuid)->placement.shift);
            break;
        case ObjectType::BOARD_HOLE:
            accu.accumulate(core.b->get_board()->holes.at(it.uuid).placement.shift);
            break;
        case ObjectType::SYMBOL_PIN:
            accu.accumulate(core.y->get_symbol_pin(it.uuid)->position);
            break;
        case ObjectType::SCHEMATIC_SYMBOL:
            accu.accumulate(core.c->get_schematic_symbol(it.uuid)->placement.shift);
            break;
        case ObjectType::BOARD_PACKAGE:
            accu.accumulate(core.b->get_board()->packages.at(it.uuid).placement.shift);
            break;
        case ObjectType::PAD:
            accu.accumulate(core.k->get_package()->pads.at(it.uuid).placement.shift);
            break;
        case ObjectType::TEXT:
            accu.accumulate(core.r->get_text(it.uuid)->placement.shift);
            break;
        case ObjectType::POLYGON_VERTEX:
            accu.accumulate(core.r->get_polygon(it.uuid)->vertices.at(it.vertex).position);
            break;
        case ObjectType::DIMENSION:
            if (it.vertex < 2) {
                auto dim = core.r->get_dimension(it.uuid);
                accu.accumulate(it.vertex == 0 ? dim->p0 : dim->p1);
            }
            break;
        case ObjectType::POLYGON_ARC_CENTER:
            accu.accumulate(core.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_center);
            break;
        case ObjectType::SHAPE:
            accu.accumulate(core.a->get_padstack()->shapes.at(it.uuid).placement.shift);
            break;
        default:;
        }
    }
    if (core.c || core.y)
        selection_center = (accu.get() / 1.25_mm) * 1.25_mm;
    else
        selection_center = accu.get();
}

ToolResponse ToolMove::begin(const ToolArgs &args)
{
    std::cout << "tool move\n";
    move_init(args.coords);

    update_selection_center();

    if (tool_id == ToolID::ROTATE || tool_id == ToolID::MIRROR_X || tool_id == ToolID::MIRROR_Y) {
        move_mirror_or_rotate(selection_center, tool_id == ToolID::ROTATE);
        if (tool_id == ToolID::MIRROR_Y) {
            move_mirror_or_rotate(selection_center, true);
            move_mirror_or_rotate(selection_center, true);
        }
        if (core.b) {
            auto brd = core.b->get_board();
            brd->expand_flags = static_cast<Board::ExpandFlags>(Board::EXPAND_AIRWIRES);
        }
        core.r->commit();
        return ToolResponse::end();
    }
    if (tool_id == ToolID::MOVE_EXACTLY) {
        auto r = imp->dialogs.ask_datum_coord("Move exactly");
        if (!r.first) {
            return ToolResponse::end();
        }
        move_do(r.second);
        if (core.b) {
            auto brd = core.b->get_board();
            brd->expand_flags = static_cast<Board::ExpandFlags>(Board::EXPAND_AIRWIRES);
        }
        core.r->commit();
        return ToolResponse::end();
    }

    collect_nets();

    update_tip();

    for (const auto &it : core.r->selection) {
        if (it.type == ObjectType::POLYGON_VERTEX || it.type == ObjectType::POLYGON_EDGE) {
            auto poly = core.r->get_polygon(it.uuid);
            if (auto plane = dynamic_cast<Plane *>(poly->usage.ptr)) {
                planes.insert(plane);
            }
        }
    }

    for (auto plane : planes) {
        plane->fragments.clear();
        plane->revision++;
    }

    int key = 0;
    switch (tool_id) {
    case ToolID::MOVE_KEY_UP:
    case ToolID::MOVE_KEY_UP_FINE:
    case ToolID::MOVE_KEY_UP_COARSE:
        key = GDK_KEY_Up;
        break;
    case ToolID::MOVE_KEY_DOWN:
    case ToolID::MOVE_KEY_DOWN_FINE:
    case ToolID::MOVE_KEY_DOWN_COARSE:
        key = GDK_KEY_Down;
        break;
    case ToolID::MOVE_KEY_LEFT:
    case ToolID::MOVE_KEY_LEFT_FINE:
    case ToolID::MOVE_KEY_LEFT_COARSE:
        key = GDK_KEY_Left;
        break;
    case ToolID::MOVE_KEY_RIGHT:
    case ToolID::MOVE_KEY_RIGHT_FINE:
    case ToolID::MOVE_KEY_RIGHT_COARSE:
        key = GDK_KEY_Right;
        break;
    default:;
    }
    if (key) {
        is_key = true;
        ToolArgs args2;
        args2.type = ToolEventType::KEY;
        args2.key = key;
        update(args2);
    }
    if (tool_id == ToolID::MOVE_KEY)
        is_key = true;

    imp->tool_bar_set_tool_name("Move");


    return ToolResponse();
}

void ToolMove::collect_nets()
{
    for (const auto &it : core.r->selection) {
        switch (it.type) {

        case ObjectType::BOARD_PACKAGE: {
            BoardPackage *pkg = &core.b->get_board()->packages.at(it.uuid);
            for (const auto &itt : pkg->package.pads) {
                if (itt.second.net)
                    nets.insert(itt.second.net->uuid);
            }
        } break;

        case ObjectType::JUNCTION: {
            auto ju = core.r->get_junction(it.uuid);
            if (ju->net)
                nets.insert(ju->net->uuid);
        } break;
        default:;
        }
    }
}

bool ToolMove::can_begin()
{
    expand_selection();
    return core.r->selection.size() > 0;
}

void ToolMove::update_tip()
{
    auto delta = get_delta();
    std::string s =
            "<b>LMB:</b>place <b>RMB:</b>cancel <b>r:</b>rotate "
            "<b>e:</b>mirror ";
    if (is_key)
        s += "<b>Enter:</b>place";
    else
        s += "<b>/:</b>restrict";
    s += " <i>" + coord_to_string(delta + key_delta, true) + " ";
    if (!is_key) {
        s += mode_to_string();
    }

    s += "</i>";
    imp->tool_bar_set_tip(s);
}


void ToolMove::do_move(const Coordi &d)
{
    move_do_cursor(d);
    if (core.b && update_airwires && nets.size()) {
        core.b->get_board()->update_airwires(true, nets);
    }
    update_tip();
}

static Coordi dir_from_arrow_key(unsigned int key)
{
    switch (key) {
    case GDK_KEY_Up:
        return {0, 1};
    case GDK_KEY_Down:
        return {0, -1};
    case GDK_KEY_Left:
        return {-1, 0};
    case GDK_KEY_Right:
        return {1, 0};
    default:
        return {0, 0};
    }
}

void ToolMove::finish()
{
    for (const auto &it : core.r->selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            core.c->get_schematic()->autoconnect_symbol(core.c->get_sheet(), core.c->get_schematic_symbol(it.uuid));
        }
    }
    if (core.c) {
        merge_selected_junctions();
    }
    if (core.b) {
        auto brd = core.b->get_board();
        brd->expand_flags = static_cast<Board::ExpandFlags>(Board::EXPAND_AIRWIRES);
        for (auto plane : planes) {
            brd->update_plane(plane);
        }
    }

    core.r->commit();
}

ToolResponse ToolMove::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (!is_key)
            do_move(args.coords);
        return ToolResponse::fast();
    }
    else if (args.type == ToolEventType::CLICK || (is_transient && args.type == ToolEventType::CLICK_RELEASE)) {
        if (args.button == 1) {
            finish();
        }
        else {
            core.r->revert();
        }
        return ToolResponse::end();
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            core.r->revert();
            return ToolResponse::end();
        }
        else if (args.key == GDK_KEY_slash) {
            cycle_mode();
            do_move(args.coords);
        }
        else if (args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
            bool rotate = args.key == GDK_KEY_r;
            update_selection_center();
            move_mirror_or_rotate(selection_center, rotate);
        }
        else if (args.key == GDK_KEY_a) {
            update_airwires ^= true;
        }
        else if (args.key == GDK_KEY_Return) {
            finish();
            return ToolResponse::end();
        }
        else {
            auto spacing = imp->get_grid_spacing();

            // TODO: Does not use actual key bindings
            switch(args.mod){
                case GDK_MOD1_MASK:
                    spacing /= 10;
                    break;
                case GDK_CONTROL_MASK:
                    spacing *= 10;
                    break;
                default:;
            }

            auto dir = dir_from_arrow_key(args.key) * spacing;
            if (dir.x || dir.y) {
                key_delta += dir;
                move_do(dir);
                update_tip();
                return ToolResponse::fast();
            }
        }
    }
    return ToolResponse();
}
} // namespace horizon
