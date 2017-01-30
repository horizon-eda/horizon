#include "tool_move.hpp"
#include "core_schematic.hpp"
#include "core_symbol.hpp"
#include "core_padstack.hpp"
#include "core_package.hpp"
#include "core_board.hpp"
#include "accumulator.hpp"
#include <iostream>

namespace horizon {
	
	ToolMove::ToolMove(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Move";
	}
	
	void ToolMove::expand_selection() {
		std::set<SelectableRef> new_sel;
		for(const auto &it : core.r->selection) {
			switch(it.type) {
				case ObjectType::LINE : {
					Line *line = core.r->get_line(it.uuid);
					new_sel.emplace(line->from.uuid, ObjectType::JUNCTION);
					new_sel.emplace(line->to.uuid, ObjectType::JUNCTION);
				} break;
				case ObjectType::POLYGON_EDGE : {
					Polygon *poly = core.r->get_polygon(it.uuid);
					auto vs = poly->get_vertices_for_edge(it.vertex);
					new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.first);
					new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vs.second);
				} break;
				
				case ObjectType::NET_LABEL : {
					auto &la = core.c->get_sheet()->net_labels.at(it.uuid);
					new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
				} break;
				case ObjectType::BUS_LABEL : {
					auto &la = core.c->get_sheet()->bus_labels.at(it.uuid);
					new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
				} break;
				case ObjectType::POWER_SYMBOL : {
					auto &ps = core.c->get_sheet()->power_symbols.at(it.uuid);
					new_sel.emplace(ps.junction->uuid, ObjectType::JUNCTION);
				} break;
				case ObjectType::BUS_RIPPER : {
					auto &rip = core.c->get_sheet()->bus_rippers.at(it.uuid);
					new_sel.emplace(rip.junction->uuid, ObjectType::JUNCTION);
				} break;

				case ObjectType::LINE_NET : {
					auto line = &core.c->get_sheet()->net_lines.at(it.uuid);
					for(auto &it_ft: {line->from, line->to}) {
						if(it_ft.is_junc()) {
							new_sel.emplace(it_ft.junc.uuid, ObjectType::JUNCTION);
						}
					}
				} break;
				case ObjectType::TRACK : {
					auto track = &core.b->get_board()->tracks.at(it.uuid);
					for(auto &it_ft: {track->from, track->to}) {
						if(it_ft.is_junc()) {
							new_sel.emplace(it_ft.junc.uuid, ObjectType::JUNCTION);
						}
					}
				} break;
				case ObjectType::VIA : {
					auto via = &core.b->get_board()->vias.at(it.uuid);
					new_sel.emplace(via->junction->uuid, ObjectType::JUNCTION);
				} break;
				case ObjectType::POLYGON : {
					auto poly = core.r->get_polygon(it.uuid);
					int i = 0;
					for(const auto &itv: poly->vertices) {
						new_sel.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, i);
						i++;
					}
				} break;

				case ObjectType::ARC : {
					Arc *arc = core.r->get_arc(it.uuid);
					new_sel.emplace(arc->from.uuid, ObjectType::JUNCTION);
					new_sel.emplace(arc->to.uuid, ObjectType::JUNCTION);
					new_sel.emplace(arc->center.uuid, ObjectType::JUNCTION);
				} break;

				case ObjectType::SCHEMATIC_SYMBOL : {
					auto sym = core.c->get_schematic_symbol(it.uuid);
					for(const auto &itt: sym->texts) {
						new_sel.emplace(itt->uuid, ObjectType::TEXT);
					}
				} break;
				case ObjectType::BOARD_PACKAGE : {
					BoardPackage *pkg = &core.b->get_board()->packages.at(it.uuid);
					for(const auto &itt: pkg->texts) {
						new_sel.emplace(itt->uuid, ObjectType::TEXT);
					}
				} break;

				default:;
			}
		}
		core.r->selection.insert(new_sel.begin(), new_sel.end());
	}
	
	void ToolMove::transform(Coordi &a, const Coordi &center, bool rotate) {
		int64_t dx = a.x - center.x;
		int64_t dy = a.y - center.y;
		if(rotate) {
			a.x = center.x + dy;
			a.y = center.y - dx;
		}
		else {
			a.x = center.x - dx;
			a.y = center.y + dy;
		}
	}

	static Orientation transform_orienation(Orientation orientation, bool rotate, bool reverse=false) {
		Orientation new_orientation;
		if(rotate) {
			if(!reverse) {
				switch(orientation) {
					case Orientation::UP:    new_orientation=Orientation::RIGHT; break;
					case Orientation::DOWN:  new_orientation=Orientation::LEFT; break;
					case Orientation::LEFT:  new_orientation=Orientation::UP; break;
					case Orientation::RIGHT: new_orientation=Orientation::DOWN; break;
				}
			}
			else {
				switch(orientation) {
					case Orientation::UP:    new_orientation=Orientation::LEFT; break;
					case Orientation::DOWN:  new_orientation=Orientation::RIGHT; break;
					case Orientation::LEFT:  new_orientation=Orientation::DOWN; break;
					case Orientation::RIGHT: new_orientation=Orientation::UP; break;
				}
			}
		}
		else {
			switch(orientation) {
				case Orientation::UP:    new_orientation=Orientation::UP; break;
				case Orientation::DOWN:  new_orientation=Orientation::DOWN; break;
				case Orientation::LEFT:  new_orientation=Orientation::RIGHT; break;
				case Orientation::RIGHT: new_orientation=Orientation::LEFT; break;
			}
		}
		return new_orientation;
	}

	void ToolMove::mirror_or_rotate(const Coordi &center,bool rotate) {
		for(const auto &it: core.r->selection) {
			switch(it.type) {
				case ObjectType::JUNCTION :
					transform(core.r->get_junction(it.uuid)->position, center, rotate);
				break;
				case ObjectType::POLYGON_VERTEX :
					transform(core.r->get_polygon(it.uuid)->vertices.at(it.vertex).position, center, rotate);
				break;
				case ObjectType::POLYGON_ARC_CENTER :
					transform(core.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_center, center, rotate);
					if(!rotate) {
						core.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_reverse ^= 1;
					}
				break;

				case ObjectType::SYMBOL_PIN: {
					SymbolPin *pin = core.y->get_symbol_pin(it.uuid);
					transform(pin->position, center, rotate);
					pin->orientation = transform_orienation(pin->orientation, rotate);

				} break;

				case ObjectType::TEXT: {
					Text *txt = core.r->get_text(it.uuid);
					transform(txt->placement.shift, center, rotate);
					bool rev = core.r->get_layers().at(txt->layer).reverse;
					if(rotate) {
						if(txt->placement.mirror) {
							txt->placement.inc_angle_deg(90);
						}
						else {
							txt->placement.inc_angle_deg(-90);
						}
					}
					else {
						txt->placement.mirror = !txt->placement.mirror;
					}


					//txt->orientation = transform_orienation(txt->orientation, rotate, rev);
				} break;

				case ObjectType::ARC :
					if(!rotate) {
						core.r->get_arc(it.uuid)->reverse();
					}
				break;
				case ObjectType::POWER_SYMBOL :
					if(!rotate) {
						auto &x = core.c->get_sheet()->power_symbols.at(it.uuid).mirror;
						x = !x;
					}
				break;

				case ObjectType::SCHEMATIC_SYMBOL : {
					SchematicSymbol *sym = core.c->get_schematic_symbol(it.uuid);
					transform(sym->placement.shift, center, rotate);
					if(rotate) {
						if(sym->placement.mirror) {
							sym->placement.inc_angle_deg(90);
						}
						else {
							sym->placement.inc_angle_deg(-90);
						}
					}
					else {
						sym->placement.mirror = !sym->placement.mirror;
					}

				}break;

				case ObjectType::BOARD_PACKAGE : {
					BoardPackage *pkg = &core.b->get_board()->packages.at(it.uuid);
					if(rotate) {
						transform(pkg->placement.shift, center, rotate);
					//	if(sym->placement.mirror) {
					//		sym->placement.angle = (sym->placement.angle+16384)%65536;
					//	}
						//else {
							pkg->placement.inc_angle_deg(-90);
					//	}
					}

				}break;



				case ObjectType::PAD : {
					Pad *pad = &core.k->get_package()->pads.at(it.uuid);
					transform(pad->placement.shift, center, rotate);
					if(rotate) {
						pad->placement.inc_angle(-90);
					}
				}break;

				case ObjectType::NET_LABEL : {
					auto sheet = core.c->get_sheet();
					auto *label = &sheet->net_labels.at(it.uuid);
					label->orientation = transform_orienation(label->orientation, rotate);
				} break;
				case ObjectType::BUS_LABEL : {
					auto sheet = core.c->get_sheet();
					auto *label = &sheet->bus_labels.at(it.uuid);
					label->orientation = transform_orienation(label->orientation, rotate);
				} break;
				default:;
			}
		}
	}

	void ToolMove::update_selection_center() {
		Accumulator<Coordi> accu;
		for(const auto &it:core.r->selection) {
			switch(it.type) {
				case ObjectType::JUNCTION :
					accu.accumulate(core.r->get_junction(it.uuid)->position);
				break;
				case ObjectType::HOLE :
					accu.accumulate(core.r->get_hole(it.uuid)->position);
				break;
				case ObjectType::SYMBOL_PIN :
					accu.accumulate(core.y->get_symbol_pin(it.uuid)->position);
				break;
				case ObjectType::SCHEMATIC_SYMBOL :
					accu.accumulate(core.c->get_schematic_symbol(it.uuid)->placement.shift);
				break;
				case ObjectType::BOARD_PACKAGE :
					accu.accumulate(core.b->get_board()->packages.at(it.uuid).placement.shift);
				break;
				case ObjectType::PAD :
					accu.accumulate(core.k->get_package()->pads.at(it.uuid).placement.shift);
				break;
				case ObjectType::TEXT :
					accu.accumulate(core.r->get_text(it.uuid)->placement.shift);
				break;
				case ObjectType::POLYGON_VERTEX :
					accu.accumulate(core.r->get_polygon(it.uuid)->vertices.at(it.vertex).position);
				break;
				case ObjectType::POLYGON_ARC_CENTER :
					accu.accumulate(core.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_center);
				break;
				default:;

			}
		}
		if(core.c || core.y)
			selection_center = (accu.get()/1.25_mm)*1.25_mm;
		else
			selection_center = accu.get();
	}

	ToolResponse ToolMove::begin(const ToolArgs &args) {
		std::cout << "tool move\n";
		last = args.coords;

		if(!can_begin()) {
			return ToolResponse::end();
		}
		update_selection_center();



		if(tool_id == ToolID::ROTATE || tool_id == ToolID::MIRROR) {
			mirror_or_rotate(selection_center, tool_id==ToolID::ROTATE);
			core.r->commit();
			return ToolResponse::end();
		}
		if(tool_id == ToolID::MOVE_EXACTLY) {
			auto r = core.r->dialogs.ask_datum_coord("Move exactly");
			if(!r.first) {
				return ToolResponse::end();
			}
			do_move(r.second);
			core.r->commit();
			return ToolResponse::end();
		}

		return ToolResponse();
	}


	bool ToolMove::can_begin() {
		expand_selection();
		return core.r->selection.size()>0;
	}

	void ToolMove::do_move(const Coordi &delta) {
		for(const auto &it:core.r->selection) {
			switch(it.type) {
				case ObjectType::JUNCTION :
					core.r->get_junction(it.uuid)->position += delta;
				break;
				case ObjectType::HOLE :
					core.r->get_hole(it.uuid)->position += delta;
				break;
				case ObjectType::SYMBOL_PIN :
					core.y->get_symbol_pin(it.uuid)->position += delta;
				break;
				case ObjectType::SCHEMATIC_SYMBOL :
					core.c->get_schematic_symbol(it.uuid)->placement.shift += delta;
				break;
				case ObjectType::BOARD_PACKAGE :
					core.b->get_board()->packages.at(it.uuid).placement.shift += delta;
				break;
				case ObjectType::PAD :
					core.k->get_package()->pads.at(it.uuid).placement.shift += delta;
				break;
				case ObjectType::TEXT :
					core.r->get_text(it.uuid)->placement.shift += delta;
				break;
				case ObjectType::POLYGON_VERTEX :
					core.r->get_polygon(it.uuid)->vertices.at(it.vertex).position += delta;
				break;
				case ObjectType::POLYGON_ARC_CENTER :
					core.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_center += delta;
				break;
				default:;

			}
		}
	}

	ToolResponse ToolMove::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			Coordi delta = args.coords - last;
			do_move(delta);
			last = args.coords;
			if(core.b) {
				core.b->get_board()->update_airwires();
			}
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				for(const auto &it:core.r->selection) {
					if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
						core.c->get_schematic()->autoconnect_symbol(core.c->get_sheet(), core.c->get_schematic_symbol(it.uuid));
					}
				}
				core.r->commit();
			}
			else {
				core.r->revert();
			}
			return ToolResponse::end();
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
			else if(args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
				bool rotate = args.key == GDK_KEY_r;
				update_selection_center();
				mirror_or_rotate(selection_center, rotate);
			}
		}
		return ToolResponse();
	}
	
}
