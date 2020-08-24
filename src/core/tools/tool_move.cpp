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
#include "pool/symbol.hpp"
#include "imp/imp_interface.hpp"
#include "util/accumulator.hpp"
#include "util/util.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolMove::ToolMove(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolHelperMove(c, tid), ToolHelperMerge(c, tid)
{
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
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            for (const auto &itt : sym->texts) {
                new_sel.emplace(itt->uuid, ObjectType::TEXT);
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
        case ObjectType::SCHEMATIC_SYMBOL:
            accu.accumulate(doc.c->get_schematic_symbol(it.uuid)->placement.shift);
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

    collect_nets();

    if (tool_id == ToolID::ROTATE || tool_id == ToolID::MIRROR_X || tool_id == ToolID::MIRROR_Y
        || tool_id == ToolID::ROTATE_CURSOR || tool_id == ToolID::MIRROR_CURSOR) {
        move_mirror_or_rotate(selection_center, tool_id == ToolID::ROTATE || tool_id == ToolID::ROTATE_CURSOR);
        if (tool_id == ToolID::MIRROR_Y) {
            move_mirror_or_rotate(selection_center, true);
            move_mirror_or_rotate(selection_center, true);
        }
        finish();
        return ToolResponse::commit();
    }
    if (tool_id == ToolID::MOVE_EXACTLY) {
        if (auto r = imp->dialogs.ask_datum_coord("Move exactly")) {
            move_do(*r);
            finish();
            return ToolResponse::commit();
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
                planes.insert(plane);
            }
        }
    }

    for (auto plane : planes) {
        plane->fragments.clear();
        plane->revision++;
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

void ToolMove::collect_nets()
{
    for (const auto &it : selection) {
        switch (it.type) {

        case ObjectType::BOARD_PACKAGE: {
            BoardPackage *pkg = &doc.b->get_board()->packages.at(it.uuid);
            for (const auto &itt : pkg->package.pads) {
                if (itt.second.net)
                    nets.insert(itt.second.net->uuid);
            }
        } break;

        case ObjectType::JUNCTION: {
            auto ju = doc.r->get_junction(it.uuid);
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


void ToolMove::do_move(const Coordi &d)
{
    move_do_cursor(d);
    if (doc.b && update_airwires && nets.size()) {
        doc.b->get_board()->update_airwires(true, nets);
    }
    update_tip();
}

void ToolMove::finish()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            doc.c->get_schematic()->autoconnect_symbol(doc.c->get_sheet(), sym);
            if (sym->component->connections.size() == 0) {
                doc.c->get_schematic()->place_bipole_on_line(doc.c->get_sheet(), sym);
            }
        }
    }
    if (doc.c) {
        merge_selected_junctions();
    }
    if (doc.b) {
        auto brd = doc.b->get_board();
        brd->expand_flags = static_cast<Board::ExpandFlags>(Board::EXPAND_AIRWIRES);
        brd->airwires_expand = nets;
        for (auto plane : planes) {
            brd->update_plane(plane);
        }
    }
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
            finish();
            return ToolResponse::commit();
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
                if (fine)
                    sp = sp / 10;
                key_delta += dir * sp;
                move_do(dir * sp);
                update_tip();
            }
        }
    }
    return ToolResponse();
}
} // namespace horizon
