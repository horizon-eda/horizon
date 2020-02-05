#include "tool_delete.hpp"
#include "core_board.hpp"
#include "core_package.hpp"
#include "core_padstack.hpp"
#include "core_schematic.hpp"
#include "core_symbol.hpp"
#include "imp/imp_interface.hpp"
#include "canvas/canvas_gl.hpp"
#include "pool/entity.hpp"
#include <iostream>

namespace horizon {

ToolDelete::ToolDelete(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolDelete::can_begin()
{
    return selection.size() > 0;
}

ToolResponse ToolDelete::begin(const ToolArgs &args)
{
    std::cout << "tool delete\n";
    std::set<SelectableRef> delete_extra;
    const auto lines = core.r->get_lines();
    const auto arcs = core.r->get_arcs();
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::JUNCTION) {
            for (const auto line : lines) {
                if (line->to.uuid == it.uuid || line->from.uuid == it.uuid) {
                    delete_extra.emplace(line->uuid, ObjectType::LINE);
                }
            }
            if (core.c) {
                for (const auto line : core.c->get_net_lines()) {
                    if (line->to.junc.uuid == it.uuid || line->from.junc.uuid == it.uuid) {
                        delete_extra.emplace(line->uuid, ObjectType::LINE_NET);
                    }
                }
            }
            if (core.b) {
                for (const auto &it_track : core.b->get_board()->tracks) {
                    if (it_track.second.to.junc.uuid == it.uuid || it_track.second.from.junc.uuid == it.uuid) {
                        delete_extra.emplace(it_track.second.uuid, ObjectType::TRACK);
                    }
                }
            }
            for (const auto arc : arcs) {
                if (arc->to.uuid == it.uuid || arc->from.uuid == it.uuid || arc->center.uuid == it.uuid) {
                    delete_extra.emplace(arc->uuid, ObjectType::ARC);
                }
            }
            if (core.c) {
                for (const auto label : core.c->get_net_labels()) {
                    if (label->junction.uuid == it.uuid) {
                        delete_extra.emplace(label->uuid, ObjectType::NET_LABEL);
                    }
                }
            }
        }
        else if (it.type == ObjectType::NET_LABEL) {
            Junction *ju = core.c->get_sheet()->net_labels.at(it.uuid).junction;
            if (ju->connection_count == 0) {
                delete_extra.emplace(ju->uuid, ObjectType::JUNCTION);
            }
        }
        else if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = core.c->get_schematic_symbol(it.uuid);
            core.c->get_schematic()->disconnect_symbol(core.c->get_sheet(), sym);
            for (const auto &it_text : sym->texts) {
                delete_extra.emplace(it_text->uuid, ObjectType::TEXT);
            }
        }
        else if (it.type == ObjectType::BOARD_PACKAGE) {
            auto pkg = &core.b->get_board()->packages.at(it.uuid);
            core.b->get_board()->disconnect_package(&core.b->get_board()->packages.at(it.uuid));
            for (const auto &it_text : pkg->texts) {
                delete_extra.emplace(it_text->uuid, ObjectType::TEXT);
            }
            for (const auto &it_li : core.b->get_board()->connection_lines) {
                if (it_li.second.from.package.uuid == it.uuid || it_li.second.to.package.uuid == it.uuid) {
                    delete_extra.emplace(it_li.second.uuid, ObjectType::CONNECTION_LINE);
                }
            }
        }
        else if (it.type == ObjectType::POLYGON_EDGE) {
            Polygon *poly = core.r->get_polygon(it.uuid);
            auto vs = poly->get_vertices_for_edge(it.vertex);
            delete_extra.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.first);
            delete_extra.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.second);
        }
    }
    delete_extra.insert(args.selection.begin(), args.selection.end());
    for (const auto &it : delete_extra) {
        if (it.type == ObjectType::LINE_NET) { // need to erase net lines before symbols
            LineNet *line = &core.c->get_sheet()->net_lines.at(it.uuid);
            core.c->get_schematic()->delete_net_line(core.c->get_sheet(), line);
        }
        else if (it.type == ObjectType::POWER_SYMBOL) { // need to erase power
                                                        // symbols before
                                                        // junctions
            auto *power_sym = &core.c->get_sheet()->power_symbols.at(it.uuid);
            auto sheet = core.c->get_sheet();
            Junction *j = power_sym->junction;
            sheet->power_symbols.erase(power_sym->uuid);

            // now, we've got a net segment with one less power symbol
            // let's see if there's still a power symbol
            // if no, extract pins on this net segment to a new net
            sheet->propagate_net_segments();
            auto ns = sheet->analyze_net_segments();
            auto &nsinfo = ns.at(j->net_segment);
            if (!nsinfo.has_power_sym) {
                auto pins = sheet->get_pins_connected_to_net_segment(j->net_segment);
                core.c->get_schematic()->block->extract_pins(pins);
            }
        }
        else if (it.type == ObjectType::BUS_RIPPER) { // need to erase bus
                                                      // rippers junctions
            auto *ripper = &core.c->get_sheet()->bus_rippers.at(it.uuid);
            auto sheet = core.c->get_sheet();
            if (ripper->connection_count == 0) {
                // just delete it
                sheet->bus_rippers.erase(ripper->uuid);
            }
            else {
                Junction *j = sheet->replace_bus_ripper(ripper);
                sheet->bus_rippers.erase(ripper->uuid);
                sheet->propagate_net_segments();
                auto ns = sheet->analyze_net_segments();
                auto &nsinfo = ns.at(j->net_segment);
                if (!nsinfo.has_label) {
                    auto pins = sheet->get_pins_connected_to_net_segment(j->net_segment);
                    core.c->get_schematic()->block->extract_pins(pins);
                }
            }

            /*Junction *j = power_sym->junction;
            sheet->bus_rippers.erase(power_sym->uuid);

            //now, we've got a net segment with one less power symbol
            //let's see if there's still a power symbol
            //if no, extract pins on this net segment to a new net
            sheet->propagate_net_segments();
            auto ns = sheet->analyze_net_segments();
            auto &nsinfo = ns.at(j->net_segment);
            if(!nsinfo.has_power_sym) {
                    auto pins =
            sheet->get_pins_connected_to_net_segment(j->net_segment);
                    core.c->get_schematic()->block->extract_pins(pins);
            }*/
        }
    }

    if (delete_extra.size() == 1) {
        auto it = delete_extra.begin();
        if (it->type == ObjectType::TRACK) {
            const auto &track = core.b->get_board()->tracks.at(it->uuid);
            const Track *other_track = nullptr;
            bool multi = false;
            for (const auto &it_ft : {track.from, track.to}) {
                if (it_ft.is_junc()) {
                    const Junction *ju = it_ft.junc;
                    for (const auto &it_track : core.b->get_board()->tracks) {
                        if (it_track.second.uuid != track.uuid
                            && (it_track.second.from.junc == ju || it_track.second.to.junc == ju)) {
                            if (other_track) { // second track, break
                                other_track = nullptr;
                                multi = true;
                                break;
                            }
                            else {
                                other_track = &it_track.second;
                            }
                        }
                    }
                    if (multi)
                        break;
                }
            }
            if (other_track) {
                selection.clear();
                selection.emplace(other_track->uuid, ObjectType::TRACK);
                imp->get_canvas()->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
            }
        }
    }

    std::set<Polygon *> polys_del;
    for (const auto &it : delete_extra) {
        switch (it.type) {
        case ObjectType::LINE:
            core.r->delete_line(it.uuid);
            break;
        case ObjectType::TRACK:
            core.b->get_board()->tracks.erase(it.uuid);
            break;
        case ObjectType::JUNCTION:
            core.r->delete_junction(it.uuid);
            break;
        case ObjectType::HOLE:
            core.r->delete_hole(it.uuid);
            break;
        case ObjectType::PAD:
            core.k->get_package()->pads.erase(it.uuid);
            break;
        case ObjectType::BOARD_HOLE:
            core.b->get_board()->holes.erase(it.uuid);
            break;
        case ObjectType::ARC:
            core.r->delete_arc(it.uuid);
            break;
        case ObjectType::SYMBOL_PIN:
            core.y->delete_symbol_pin(it.uuid);
            break;
        case ObjectType::BOARD_PACKAGE:
            core.b->get_board()->packages.erase(it.uuid);
            break;
        case ObjectType::SCHEMATIC_SYMBOL: {
            SchematicSymbol *schsym = core.c->get_schematic_symbol(it.uuid);
            Component *comp = schsym->component.ptr;
            core.c->delete_schematic_symbol(it.uuid);
            Schematic *sch = core.c->get_schematic();
            bool found = false;
            for (auto &it_sheet : sch->sheets) {
                for (auto &it_sym : it_sheet.second.symbols) {
                    if (it_sym.second.component.uuid == comp->uuid) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            if (found == false) {
                if (comp->entity->gates.size() > 1) {
                    imp->tool_bar_flash("deleted last gate, deleting component");
                }
                sch->block->components.erase(comp->uuid);
                comp = nullptr;
            }
        } break;
        case ObjectType::NET_LABEL: {
            core.c->get_sheet()->net_labels.erase(it.uuid);
        } break;
        case ObjectType::BUS_LABEL: {
            core.c->get_sheet()->bus_labels.erase(it.uuid);
        } break;
        case ObjectType::VIA:
            core.b->get_board()->vias.erase(it.uuid);
            break;
        case ObjectType::SHAPE:
            core.a->get_padstack()->shapes.erase(it.uuid);
            break;
        case ObjectType::TEXT:
            core.r->delete_text(it.uuid);
            break;
        case ObjectType::POLYGON_VERTEX: {
            Polygon *poly = core.r->get_polygon(it.uuid);
            poly->vertices.at(it.vertex).remove = true;
            polys_del.insert(poly);
        } break;
        case ObjectType::POLYGON_ARC_CENTER: {
            Polygon *poly = core.r->get_polygon(it.uuid);
            poly->vertices.at(it.vertex).type = Polygon::Vertex::Type::LINE;
            polys_del.insert(poly);
        } break;
        case ObjectType::DIMENSION: {
            core.r->delete_dimension(it.uuid);
        } break;
        case ObjectType::CONNECTION_LINE: {
            core.b->get_board()->connection_lines.erase(it.uuid);
        } break;


        case ObjectType::INVALID:
            break;
        default:;
        }
    }
    for (auto it : polys_del) {
        it->vertices.erase(
                std::remove_if(it->vertices.begin(), it->vertices.end(), [](const auto &x) { return x.remove; }),
                it->vertices.end());
        if (!it->is_valid()) {
            core.r->delete_polygon(it->uuid);
        }
    }
    if (core.b) {
        auto brd = core.b->get_board();
        brd->expand_flags = static_cast<Board::ExpandFlags>(Board::EXPAND_AIRWIRES | Board::EXPAND_PROPAGATE_NETS);
    }

    return ToolResponse::commit();
}
ToolResponse ToolDelete::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
