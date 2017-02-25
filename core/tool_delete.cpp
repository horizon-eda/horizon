#include "tool_delete.hpp"
#include "core_schematic.hpp"
#include "core_symbol.hpp"
#include "core_padstack.hpp"
#include "core_package.hpp"
#include "core_board.hpp"
#include <iostream>

namespace horizon {
	
	ToolDelete::ToolDelete(Core *c, ToolID tid) : ToolBase(c,tid) {
	}

	bool ToolDelete::can_begin() {
		return core.r->selection.size()>0;

	}

	ToolResponse ToolDelete::begin(const ToolArgs &args) {
		std::cout << "tool delete\n";
		std::set<SelectableRef> delete_extra;
		const auto lines = core.r->get_lines();
		const auto arcs = core.r->get_arcs();
		for(const auto &it : args.selection) {
			if(it.type == ObjectType::JUNCTION) {
				for(const auto line: lines) {
					if(line->to.uuid == it.uuid || line->from.uuid == it.uuid) {
						delete_extra.emplace(line->uuid, ObjectType::LINE);
					}
				}
				if(core.c) {
					for(const auto line: core.c->get_net_lines()) {
						if(line->to.junc.uuid == it.uuid || line->from.junc.uuid == it.uuid) {
							delete_extra.emplace(line->uuid, ObjectType::LINE_NET);
						}
					}
				}
				if(core.b) {
					for(const auto track: core.b->get_tracks()) {
						if(track->to.junc.uuid == it.uuid || track->from.junc.uuid == it.uuid) {
							delete_extra.emplace(track->uuid, ObjectType::TRACK);
						}
					}
				}
				for(const auto arc: arcs) {
					if(arc->to.uuid == it.uuid || arc->from.uuid == it.uuid || arc->center.uuid == it.uuid) {
						delete_extra.emplace(arc->uuid, ObjectType::ARC);
					}
				}
				if(core.c) {
					for(const auto label: core.c->get_net_labels()) {
						if(label->junction.uuid == it.uuid) {
							delete_extra.emplace(label->uuid, ObjectType::NET_LABEL);
						}
					}
				}
			}
			else if(it.type == ObjectType::NET_LABEL) {
				Junction *ju = core.c->get_sheet()->net_labels.at(it.uuid).junction;
				if(ju->connection_count == 0) {
					delete_extra.emplace(ju->uuid, ObjectType::JUNCTION);
				}
			}
			else if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				auto sym = core.c->get_schematic_symbol(it.uuid);
				core.c->get_schematic()->disconnect_symbol(core.c->get_sheet(), sym);
				for(const auto &it_text: sym->texts) {
					delete_extra.emplace(it_text->uuid, ObjectType::TEXT);
				}
			}
			else if(it.type == ObjectType::BOARD_PACKAGE) {
				core.b->get_board()->disconnect_package(&core.b->get_board()->packages.at(it.uuid));
			}
			else if(it.type == ObjectType::POLYGON_EDGE) {
				Polygon *poly = core.r->get_polygon(it.uuid);
				auto vs = poly->get_vertices_for_edge(it.vertex);
				delete_extra.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.first);
				delete_extra.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.second);
			}
		}
		delete_extra.insert(args.selection.begin(), args.selection.end());
		for(const auto &it: delete_extra) {
			if(it.type == ObjectType::LINE_NET) { //need to erase net lines before symbols
				LineNet *line  = &core.c->get_sheet()->net_lines.at(it.uuid);
				if(line->net) {
					for(auto &it_ft: {line->from, line->to}) {
						if(it_ft.is_pin()) {
							UUIDPath<2> conn_path(it_ft.symbol->gate.uuid, it_ft.pin->uuid);
							if(it_ft.symbol->component->connections.count(conn_path) &&
							   it_ft.pin->connected_net_lines.size()<=1) {
								it_ft.symbol->component->connections.erase(conn_path);
							}
							//prevents error in update_refs
							it_ft.pin->connected_net_lines.erase(it.uuid);
						}
					}
				}
				auto from = line->from;
				auto to = line->to;
				Net *net = line->net;
				core.c->get_sheet()->net_lines.erase(it.uuid);
				core.c->get_sheet()->propagate_net_segments();
				if(net) {
					UUID from_net_segment = from.get_net_segment();
					UUID to_net_segment = to.get_net_segment();
					if(from_net_segment != to_net_segment) {
						auto pins_from = core.c->get_sheet()->get_pins_connected_to_net_segment(from_net_segment);
						auto pins_to = core.c->get_sheet()->get_pins_connected_to_net_segment(to_net_segment);
						std::cout << "!!!net split" << std::endl;
						Block *b = core.c->get_schematic()->block;
						if(!net->is_named() && !net->is_power && !net->is_bussed) {
							//net is unnamed, not bussed and not a power net, user does't care which pins get extracted
							core.r->tool_bar_flash("net split");
							b->extract_pins(pins_to);
						}
						else if(net->is_power) {
							auto ns_info = core.c->get_sheet()->analyze_net_segments();
							if(ns_info.count(from_net_segment) && ns_info.count(to_net_segment)) {
								auto &inf_from = ns_info.at(from_net_segment);
								auto &inf_to = ns_info.at(to_net_segment);
								if(inf_from.has_power_sym && inf_to.has_power_sym) {
									//both have label, don't need to split net
								}
								else if(inf_from.has_power_sym && !inf_to.has_power_sym) {
									//from has label, to not, extracts pins on to net segment
									core.r->tool_bar_flash("net split");
									b->extract_pins(pins_to);
								}
								else if(!inf_from.has_power_sym && inf_to.has_power_sym) {
									//to has label, from not, extract pins on from segment
									core.r->tool_bar_flash("net split");
									b->extract_pins(pins_from);
								}
								else {
									core.r->tool_bar_flash("net split");
									b->extract_pins(pins_from);
								}
							}
						}
						else {
							auto ns_info = core.c->get_sheet()->analyze_net_segments();
							if(ns_info.count(from_net_segment) && ns_info.count(to_net_segment)) {
								auto &inf_from = ns_info.at(from_net_segment);
								auto &inf_to = ns_info.at(to_net_segment);
								if(inf_from.has_label && inf_to.has_label) {
									//both have label, don't need to split net
								}
								else if(inf_from.has_label && !inf_to.has_label) {
									//from has label, to not, extracts pins on to net segment
									core.r->tool_bar_flash("net split");
									b->extract_pins(pins_to);
								}
								else if(!inf_from.has_label && inf_to.has_label) {
									//to has label, from not, extract pins on from segment
									core.r->tool_bar_flash("net split");
									b->extract_pins(pins_from);
								}
								else if(!inf_from.has_label && !inf_to.has_label) {
									//both segments are unlabeled, so don't care
									core.r->tool_bar_flash("net split");
									b->extract_pins(pins_from);
								}
							}
						}
					}
				}
			}
			else if(it.type == ObjectType::POWER_SYMBOL) { //need to erase power symbols before junctions
				auto *power_sym = &core.c->get_sheet()->power_symbols.at(it.uuid);
				auto sheet = core.c->get_sheet();
				Junction *j = power_sym->junction;
				sheet->power_symbols.erase(power_sym->uuid);

				//now, we've got a net segment with one less power symbol
				//let's see if there's still a power symbol
				//if no, extract pins on this net segment to a new net
				sheet->propagate_net_segments();
				auto ns = sheet->analyze_net_segments();
				auto &nsinfo = ns.at(j->net_segment);
				if(!nsinfo.has_power_sym) {
					auto pins = sheet->get_pins_connected_to_net_segment(j->net_segment);
					core.c->get_schematic()->block->extract_pins(pins);
				}
			}
			else if(it.type == ObjectType::BUS_RIPPER) { //need to erase bus rippers junctions
				auto *ripper = &core.c->get_sheet()->bus_rippers.at(it.uuid);
				auto sheet = core.c->get_sheet();
				if(ripper->connection_count == 0) {
					//just delete it
					sheet->bus_rippers.erase(ripper->uuid);
				}
				else {
					Junction *j = sheet->replace_bus_ripper(ripper);
					sheet->bus_rippers.erase(ripper->uuid);
					sheet->propagate_net_segments();
					auto ns = sheet->analyze_net_segments();
					auto &nsinfo = ns.at(j->net_segment);
					if(!nsinfo.has_label) {
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
					auto pins = sheet->get_pins_connected_to_net_segment(j->net_segment);
					core.c->get_schematic()->block->extract_pins(pins);
				}*/
			}

		}

		std::set<Polygon*> polys_del;
		for(const auto &it: delete_extra) {
			switch(it.type) {
				case ObjectType::LINE :
					core.r->delete_line(it.uuid);
				break;
				case ObjectType::TRACK :
					core.b->get_board()->tracks.erase(it.uuid);
				break;
				case ObjectType::JUNCTION :
					core.r->delete_junction(it.uuid);
				break;
				case ObjectType::HOLE :
					core.r->delete_hole(it.uuid);
				break;
				case ObjectType::PAD :
					core.k->get_package()->pads.erase(it.uuid);
				break;
				case ObjectType::ARC :
					core.r->delete_arc(it.uuid);
				break;
				case ObjectType::SYMBOL_PIN :
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
					for(auto &it_sheet:sch->sheets) {
						for(auto &it_sym: it_sheet.second.symbols) {
							if(it_sym.second.component.uuid == comp->uuid) {
								found = true;
								break;
							}
						}
						if(found) {
							break;
						}
					}
					if(found == false) {
						if((comp->entity->gates.size()==1) || core.r->dialogs.ask_delete_component(comp)) {
							sch->block->components.erase(comp->uuid);
							comp = nullptr;
						}
					}
				}break;
				case ObjectType::NET_LABEL : {
					core.c->get_sheet()->net_labels.erase(it.uuid);
				}break;
				case ObjectType::BUS_LABEL : {
					core.c->get_sheet()->bus_labels.erase(it.uuid);
				}break;
				case ObjectType::VIA :
					core.b->get_board()->vias.erase(it.uuid);
				break;
				case ObjectType::TEXT :
					core.r->delete_text(it.uuid);
				break;
				case ObjectType::POLYGON_VERTEX : {
					Polygon *poly = core.r->get_polygon(it.uuid);
					poly->vertices.at(it.vertex).remove = true;
					polys_del.insert(poly);
				} break;
				case ObjectType::POLYGON_ARC_CENTER : {
					Polygon *poly = core.r->get_polygon(it.uuid);
					poly->vertices.at(it.vertex).type = Polygon::Vertex::Type::LINE;
				} break;

				case ObjectType::INVALID :
				break;
			}
		}
		for(auto it: polys_del) {
			it->vertices.erase(std::remove_if(it->vertices.begin(), it->vertices.end(), [](const auto &x){return x.remove;}), it->vertices.end());
			if(!it->is_valid()) {
				core.r->delete_polygon(it->uuid);
			}
		}

		core.r->commit();
		return ToolResponse::end();
	}
	ToolResponse ToolDelete::update(const ToolArgs &args) {
		return ToolResponse();
	}
	
}
