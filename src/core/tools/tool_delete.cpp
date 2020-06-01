#include "tool_delete.hpp"
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
    const auto lines = doc.r->get_lines();
    const auto arcs = doc.r->get_arcs();
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::JUNCTION) {
            for (const auto line : lines) {
                if (line->to.uuid == it.uuid || line->from.uuid == it.uuid) {
                    delete_extra.emplace(line->uuid, ObjectType::LINE);
                }
            }
            if (doc.c) {
                for (const auto line : doc.c->get_net_lines()) {
                    if (line->to.junc.uuid == it.uuid || line->from.junc.uuid == it.uuid) {
                        delete_extra.emplace(line->uuid, ObjectType::LINE_NET);
                    }
                }
                for (const auto label : doc.c->get_net_labels()) {
                    if (label->junction.uuid == it.uuid) {
                        delete_extra.emplace(label->uuid, ObjectType::NET_LABEL);
                    }
                }
                for (const auto &it_rip : doc.c->get_sheet()->bus_rippers) {
                    if (it_rip.second.junction->uuid == it.uuid) {
                        delete_extra.emplace(it_rip.first, ObjectType::BUS_RIPPER);
                    }
                }
            }
            if (doc.b) {
                for (const auto &it_track : doc.b->get_board()->tracks) {
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
        }
        else if (it.type == ObjectType::NET_LABEL) {
            Junction *ju = doc.c->get_sheet()->net_labels.at(it.uuid).junction;
            if (ju->connection_count == 0) {
                delete_extra.emplace(ju->uuid, ObjectType::JUNCTION);
            }
        }
        else if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            doc.c->get_schematic()->disconnect_symbol(doc.c->get_sheet(), sym);
            for (const auto &it_text : sym->texts) {
                delete_extra.emplace(it_text->uuid, ObjectType::TEXT);
            }
        }
        else if (it.type == ObjectType::BOARD_PACKAGE) {
            auto pkg = &doc.b->get_board()->packages.at(it.uuid);
            doc.b->get_board()->disconnect_package(&doc.b->get_board()->packages.at(it.uuid));
            for (const auto &it_text : pkg->texts) {
                delete_extra.emplace(it_text->uuid, ObjectType::TEXT);
            }
            for (const auto &it_li : doc.b->get_board()->connection_lines) {
                if (it_li.second.from.package.uuid == it.uuid || it_li.second.to.package.uuid == it.uuid) {
                    delete_extra.emplace(it_li.second.uuid, ObjectType::CONNECTION_LINE);
                }
            }
        }
        else if (it.type == ObjectType::POLYGON_EDGE) {
            Polygon *poly = doc.r->get_polygon(it.uuid);
            auto vs = poly->get_vertices_for_edge(it.vertex);
            delete_extra.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.first);
            delete_extra.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.second);
        }
    }
    delete_extra.insert(args.selection.begin(), args.selection.end());
    for (const auto &it : delete_extra) {
        if (it.type == ObjectType::LINE_NET) { // need to erase net lines before symbols
            LineNet *line = &doc.c->get_sheet()->net_lines.at(it.uuid);
            doc.c->get_schematic()->delete_net_line(doc.c->get_sheet(), line);
        }
        else if (it.type == ObjectType::POWER_SYMBOL) { // need to erase power
                                                        // symbols before
                                                        // junctions
            auto *power_sym = &doc.c->get_sheet()->power_symbols.at(it.uuid);
            auto sheet = doc.c->get_sheet();
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
                doc.c->get_schematic()->block->extract_pins(pins);
            }
        }
        else if (it.type == ObjectType::BUS_RIPPER) { // need to erase bus
                                                      // rippers junctions
            auto *ripper = &doc.c->get_sheet()->bus_rippers.at(it.uuid);
            auto sheet = doc.c->get_sheet();
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
                    doc.c->get_schematic()->block->extract_pins(pins);
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
                    doc.c->get_schematic()->block->extract_pins(pins);
            }*/
        }
    }

    if (delete_extra.size() == 1) {
        auto it = delete_extra.begin();
        if (it->type == ObjectType::TRACK) {
            const auto &track = doc.b->get_board()->tracks.at(it->uuid);
            const Track *other_track = nullptr;
            const Via *other_via = nullptr;
            bool multi = false;
            for (const auto &it_ft : {track.from, track.to}) {
                if (it_ft.is_junc()) {
                    const Junction *ju = it_ft.junc;
                    for (const auto &it_track : doc.b->get_board()->tracks) {
                        if (it_track.second.uuid != track.uuid
                            && (it_track.second.from.junc == ju || it_track.second.to.junc == ju)) {
                            if (other_track) { // second track, break
                                other_track = nullptr;
                                multi = true;
                                break;
                            }
                            else {
                                other_track = &it_track.second;
                                if (ju->has_via) {
                                    for (const auto &it_via : doc.b->get_board()->vias) {
                                        if (it_via.second.junction == ju) {
                                            other_via = &it_via.second;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (multi)
                        break;
                }
            }
            if (other_track) {
                selection.clear();
                if (other_via) {
                    selection.emplace(other_via->uuid, ObjectType::VIA);
                }
                else {
                    selection.emplace(other_track->uuid, ObjectType::TRACK);
                }
                imp->get_canvas()->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
            }
        }
        else if (it->type == ObjectType::VIA) {
            const Junction *ju = doc.b->get_board()->vias.at(it->uuid).junction;
            const Track *other_track = nullptr;
            for (const auto &it_track : doc.b->get_board()->tracks) {
                if (it_track.second.from.junc == ju || it_track.second.to.junc == ju) {
                    if (other_track) { // second track, break
                        other_track = nullptr;
                        break;
                    }
                    else {
                        other_track = &it_track.second;
                    }
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
            doc.r->delete_line(it.uuid);
            break;
        case ObjectType::TRACK:
            doc.b->get_board()->tracks.erase(it.uuid);
            break;
        case ObjectType::JUNCTION:
            doc.r->delete_junction(it.uuid);
            break;
        case ObjectType::HOLE:
            doc.r->delete_hole(it.uuid);
            break;
        case ObjectType::PAD:
            doc.k->get_package()->pads.erase(it.uuid);
            break;
        case ObjectType::BOARD_HOLE:
            doc.b->get_board()->holes.erase(it.uuid);
            break;
        case ObjectType::ARC:
            doc.r->delete_arc(it.uuid);
            break;
        case ObjectType::SYMBOL_PIN:
            doc.y->delete_symbol_pin(it.uuid);
            break;
        case ObjectType::BOARD_PACKAGE:
            doc.b->get_board()->packages.erase(it.uuid);
            break;
        case ObjectType::BOARD_PANEL:
            doc.b->get_board()->board_panels.erase(it.uuid);
            break;
        case ObjectType::PICTURE:
            doc.r->delete_picture(it.uuid);
            break;
        case ObjectType::SCHEMATIC_SYMBOL: {
            SchematicSymbol *schsym = doc.c->get_schematic_symbol(it.uuid);
            Component *comp = schsym->component.ptr;
            doc.c->delete_schematic_symbol(it.uuid);
            Schematic *sch = doc.c->get_schematic();
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
            doc.c->get_sheet()->net_labels.erase(it.uuid);
        } break;
        case ObjectType::BUS_LABEL: {
            doc.c->get_sheet()->bus_labels.erase(it.uuid);
        } break;
        case ObjectType::VIA:
            doc.b->get_board()->vias.erase(it.uuid);
            break;
        case ObjectType::SHAPE:
            doc.a->get_padstack()->shapes.erase(it.uuid);
            break;
        case ObjectType::TEXT:
            doc.r->delete_text(it.uuid);
            break;
        case ObjectType::POLYGON_VERTEX: {
            Polygon *poly = doc.r->get_polygon(it.uuid);
            poly->vertices.at(it.vertex).remove = true;
            polys_del.insert(poly);
        } break;
        case ObjectType::POLYGON_ARC_CENTER: {
            Polygon *poly = doc.r->get_polygon(it.uuid);
            poly->vertices.at(it.vertex).type = Polygon::Vertex::Type::LINE;
            polys_del.insert(poly);
        } break;
        case ObjectType::DIMENSION: {
            doc.r->delete_dimension(it.uuid);
        } break;
        case ObjectType::CONNECTION_LINE: {
            doc.b->get_board()->connection_lines.erase(it.uuid);
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
            doc.r->delete_polygon(it->uuid);
        }
    }
    if (doc.b) {
        auto brd = doc.b->get_board();
        brd->expand_flags = static_cast<Board::ExpandFlags>(Board::EXPAND_AIRWIRES | Board::EXPAND_PROPAGATE_NETS);
    }

    return ToolResponse::commit();
}
ToolResponse ToolDelete::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
