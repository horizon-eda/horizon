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
    last = args.coords;
    origin = args.coords;

    update_selection_center();

    if (tool_id == ToolID::ROTATE || tool_id == ToolID::MIRROR_X || tool_id == ToolID::MIRROR_Y) {
        move_mirror_or_rotate(selection_center, tool_id == ToolID::ROTATE);
        if (tool_id == ToolID::MIRROR_Y) {
            move_mirror_or_rotate(selection_center, true);
            move_mirror_or_rotate(selection_center, true);
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
        core.r->commit();
        return ToolResponse::end();
    }
    update_tip();
    return ToolResponse();
}

bool ToolMove::can_begin()
{
    expand_selection();
    return core.r->selection.size() > 0;
}

void ToolMove::update_tip()
{
    auto delta = last - origin;
    std::string s =
            "<b>LMB:</b>place <b>RMB:</b>cancel <b>r:</b>rotate "
            "<b>e:</b>mirror <b>/:</b>restrict <i>"
            + coord_to_string(delta, true) + " ";
    switch (mode) {
    case Mode::ARB:
        s += "any direction";
        break;
    case Mode::X:
        s += "X only";
        break;
    case Mode::Y:
        s += "Y only";
        break;
    }

    s += "</i>";
    imp->tool_bar_set_tip(s);
}

Coordi ToolMove::get_coord(const Coordi &c)
{
    switch (mode) {
    case Mode::ARB:
        return c;
    case Mode::X:
        return {c.x, origin.y};
    case Mode::Y:
        return {origin.x, c.y};
    }
    return c;
}

void ToolMove::do_move(const Coordi &d)
{
    auto c = get_coord(d);
    Coordi delta = c - last;
    move_do(delta);
    last = c;
    if (core.b) {
        core.b->get_board()->update_airwires(true);
    }
    update_tip();
}

ToolResponse ToolMove::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        do_move(args.coords);
    }
    else if (args.type == ToolEventType::CLICK || (is_transient && args.type == ToolEventType::CLICK_RELEASE)) {
        if (args.button == 1) {
            for (const auto &it : core.r->selection) {
                if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                    core.c->get_schematic()->autoconnect_symbol(core.c->get_sheet(),
                                                                core.c->get_schematic_symbol(it.uuid));
                }
            }
            if (core.c) {
                merge_selected_junctions();
            }
            core.r->commit();
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
            switch (mode) {
            case Mode::ARB:
                mode = Mode::X;
                break;
            case Mode::X:
                mode = Mode::Y;
                break;
            case Mode::Y:
                mode = Mode::ARB;
                break;
            }
            do_move(args.coords);
        }
        else if (args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
            bool rotate = args.key == GDK_KEY_r;
            update_selection_center();
            move_mirror_or_rotate(selection_center, rotate);
        }
    }
    return ToolResponse();
}
} // namespace horizon
