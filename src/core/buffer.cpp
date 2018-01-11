#include "buffer.hpp"
#include "core_package.hpp"
#include "core_schematic.hpp"
#include "core_symbol.hpp"
#include "core_padstack.hpp"
#include "pool/entity.hpp"
#include "util/util.hpp"

namespace horizon {
	Buffer::Buffer(Core *co):core(co),net_class_dummy(UUID::random()) {}

	void Buffer::clear() {
		texts.clear();
		lines.clear();
		pins.clear();
		junctions.clear();
		arcs.clear();
		pads.clear();
		holes.clear();
		polygons.clear();
		components.clear();
		symbols.clear();
		shapes.clear();
		net_lines.clear();
		power_symbols.clear();
		net_labels.clear();
	}

	void Buffer::load_from_symbol(std::set<SelectableRef> selection) {
		clear();
		std::set<SelectableRef> new_sel;
		for(const auto &it : selection) {
			switch(it.type) {
				case ObjectType::LINE : {
					Line *line = core.r->get_line(it.uuid);
					new_sel.emplace(line->from.uuid, ObjectType::JUNCTION);
					new_sel.emplace(line->to.uuid, ObjectType::JUNCTION);
				} break;
				case ObjectType::POLYGON_EDGE : {
					Polygon *poly = core.r->get_polygon(it.uuid);
					new_sel.emplace(poly->uuid, ObjectType::POLYGON);
				} break;
				case ObjectType::POLYGON_VERTEX : {
					Polygon *poly = core.r->get_polygon(it.uuid);
					new_sel.emplace(poly->uuid, ObjectType::POLYGON);
				} break;
				case ObjectType::ARC : {
					Arc *arc = core.r->get_arc(it.uuid);
					new_sel.emplace(arc->from.uuid, ObjectType::JUNCTION);
					new_sel.emplace(arc->to.uuid, ObjectType::JUNCTION);
					new_sel.emplace(arc->center.uuid, ObjectType::JUNCTION);
				} break;
				case ObjectType::SCHEMATIC_SYMBOL : {
					auto &sym = core.c->get_sheet()->symbols.at(it.uuid);
					new_sel.emplace(sym.component->uuid , ObjectType::COMPONENT);
					for(const auto &it_txt: sym.texts) {
						new_sel.emplace(it_txt->uuid, ObjectType::TEXT);
					}
				} break;
				case ObjectType::LINE_NET : {
					auto line = &core.c->get_sheet()->net_lines.at(it.uuid);
					for(auto &it_ft: {line->from, line->to}) {
						if(it_ft.is_junc()) {
							new_sel.emplace(it_ft.junc.uuid, ObjectType::JUNCTION);
						}
						if(line->net)
							new_sel.emplace(line->net->uuid, ObjectType::NET);
					}
				} break;
				case ObjectType::NET_LABEL : {
					auto &la = core.c->get_sheet()->net_labels.at(it.uuid);
					new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
				} break;
				case ObjectType::POWER_SYMBOL : {
					auto &ps = core.c->get_sheet()->power_symbols.at(it.uuid);
					new_sel.emplace(ps.net->uuid, ObjectType::NET);
					new_sel.emplace(ps.junction->uuid, ObjectType::JUNCTION);
				} break;

				/*
				case ObjectType::BUS_LABEL : {
					auto &la = core.c->get_sheet()->bus_labels.at(it.uuid);
					new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
				} break;

				case ObjectType::BUS_RIPPER : {
					auto &rip = core.c->get_sheet()->bus_rippers.at(it.uuid);
					new_sel.emplace(rip.junction->uuid, ObjectType::JUNCTION);
				} break;


				*/


				default:;
			}
		}
		selection.insert(new_sel.begin(), new_sel.end());

		//don't need nets from components
		/*new_sel.clear();
		for(const auto &it : selection) {
			if(it.type == ObjectType::COMPONENT) {
				auto &comp = core.c->get_schematic()->block->components.at(it.uuid);
				for(const auto &it_conn: comp.connections) {
					new_sel.emplace(it_conn.second.net->uuid, ObjectType::NET);
				}
			}
		}
		selection.insert(new_sel.begin(), new_sel.end());*/



		for(const auto &it: selection) {
			if(it.type == ObjectType::TEXT) {
				auto x = core.r->get_text(it.uuid);
				texts.emplace(x->uuid, *x);
			}
			else if(it.type == ObjectType::JUNCTION) {
				auto x = core.r->get_junction(it.uuid);
				junctions.emplace(x->uuid, *x);
			}
			else if(it.type == ObjectType::PAD) {
				auto x = &core.k->get_package()->pads.at(it.uuid);
				pads.emplace(x->uuid, *x);
			}
			else if(it.type == ObjectType::NET) {
				auto &x = core.c->get_schematic()->block->nets.at(it.uuid);
				auto net = &nets.emplace(x.uuid, x).first->second;
				net->net_class = &net_class_dummy;
			}
			else if(it.type == ObjectType::SYMBOL_PIN) {
				auto x = core.y->get_symbol_pin(it.uuid);
				pins.emplace(x->uuid, *x);
			}
			else if(it.type == ObjectType::HOLE) {
				auto x = core.r->get_hole(it.uuid);
				holes.emplace(x->uuid, *x);
			}
			else if(it.type == ObjectType::POLYGON) {
				auto x = core.r->get_polygon(it.uuid);
				polygons.emplace(x->uuid, *x);
			}
			else if(it.type == ObjectType::SHAPE) {
				auto x = &core.a->get_padstack()->shapes.at(it.uuid);
				shapes.emplace(x->uuid, *x);
			}

		}
		for(const auto &it: selection) {
			if(it.type == ObjectType::LINE) {
				auto x = core.r->get_line(it.uuid);
				auto &li = lines.emplace(x->uuid, *x).first->second;
				li.from = &junctions.at(li.from.uuid);
				li.to = &junctions.at(li.to.uuid);
			}
			else if(it.type == ObjectType::ARC) {
				auto x = core.r->get_arc(it.uuid);
				auto &arc = arcs.emplace(x->uuid, *x).first->second;
				arc.from = &junctions.at(arc.from.uuid);
				arc.to = &junctions.at(arc.to.uuid);
				arc.center = &junctions.at(arc.center.uuid);
			}
			else if(it.type == ObjectType::COMPONENT) {
				auto &x = core.c->get_schematic()->block->components.at(it.uuid);
				auto &comp = components.emplace(x.uuid, x).first->second;
				comp.refdes = comp.entity->prefix + "?";
			}
		}
		for(const auto &it: selection) {
			if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				auto &x = core.c->get_sheet()->symbols.at(it.uuid);
				auto &sym = symbols.emplace(x.uuid, x).first->second;
				for(auto &it_txt: sym.texts) {
					it_txt.update(texts);
				}
				sym.component.update(components);
				sym.gate.update(sym.component->entity->gates);
			}
		}
		for(const auto &it: selection) {
			if(it.type == ObjectType::LINE_NET) {
				auto &x = core.c->get_sheet()->net_lines.at(it.uuid);
				bool valid = true;
				for(auto &it_ft: {x.from, x.to}) {
					if(it_ft.is_bus_ripper()) {
						valid = false;
					}
					else if(it_ft.is_pin()) {
						if(symbols.count(it_ft.symbol->uuid) == 0) {
							valid = false;
						}
					}
				}
				if(valid) {
					auto &li = net_lines.emplace(x.uuid, x).first->second;
					li.net.update(nets);
					auto update_conn = [this](LineNet::Connection &c) {
						if(c.symbol) {
							c.symbol.update(symbols);
							c.pin.update(c.symbol->symbol.pins);
						}
						if(c.junc) {
							c.junc.update(junctions);
						}
					};
					update_conn(li.from);
					update_conn(li.to);
				}
			}
			if(it.type == ObjectType::NET_LABEL) {
				auto x = &core.c->get_sheet()->net_labels.at(it.uuid);
				auto &la = net_labels.emplace(x->uuid, *x).first->second;
				la.junction.update(junctions);
			}
			if(it.type == ObjectType::POWER_SYMBOL) {
				auto x = &core.c->get_sheet()->power_symbols.at(it.uuid);
				auto &ps = power_symbols.emplace(x->uuid, *x).first->second;
				ps.junction.update(junctions);
				ps.net.update(nets);
			}
		}
		for(const auto &it: selection) {
			if(it.type == ObjectType::COMPONENT) {
				auto comp = &components.at(it.uuid);
				map_erase_if(comp->connections, [this](auto &x){
					return nets.count(x.second.net.uuid)==0;
				});
				for(auto &it_conn: comp->connections) {
					it_conn.second.net.update(nets);
				}
				map_erase_if(comp->connections, [this, comp](auto &x){
					for(auto &it_line: net_lines) {
						if(it_line.second.net == x.second.net) {
							for(auto &it_ft: {it_line.second.from, it_line.second.to}) {
								if(it_ft.is_pin()) {
									for(const auto &it_sym: symbols) {
										if(it_sym.second.component == comp && it_ft.symbol == &it_sym.second) {
											return false;
										}
									}
								}
							}
							//return true;
						}
					}
					return true;
				});

			}
		}


	}

	json Buffer::serialize() {
		json j;
		j["texts"] = json::object();
		for(const auto &it: texts) {
			j["texts"][(std::string)it.first] = it.second.serialize();
		}
		j["junctions"] = json::object();
		for(const auto &it: junctions) {
			j["junctions"][(std::string)it.first] = it.second.serialize();
		}
		j["lines"] = json::object();
		for(const auto &it: lines) {
			j["lines"][(std::string)it.first] = it.second.serialize();
		}
		j["arcs"] = json::object();
		for(const auto &it: arcs) {
			j["arcs"][(std::string)it.first] = it.second.serialize();
		}
		j["pads"] = json::object();
		for(const auto &it: pads) {
			j["pads"][(std::string)it.first] = it.second.serialize();
		}
		j["holes"] = json::object();
		for(const auto &it: holes) {
			j["holes"][(std::string)it.first] = it.second.serialize();
		}
		j["polygons"] = json::object();
		for(const auto &it: polygons) {
			j["polygons"][(std::string)it.first] = it.second.serialize();
		}
		j["components"] = json::object();
		for(const auto &it: components) {
			j["components"][(std::string)it.first] = it.second.serialize();
		}
		j["symbols"] = json::object();
		for(const auto &it: symbols) {
			j["symbols"][(std::string)it.first] = it.second.serialize();
		}
		j["shapes"] = json::object();
		for(const auto &it: shapes) {
			j["shapes"][(std::string)it.first] = it.second.serialize();
		}
		j["nets"] = json::object();
		for(const auto &it: nets) {
			j["nets"][(std::string)it.first] = it.second.serialize();
		}
		j["net_lines"] = json::object();
		for(const auto &it: net_lines) {
			j["net_lines"][(std::string)it.first] = it.second.serialize();
		}
		j["net_labels"] = json::object();
		for(const auto &it: net_labels) {
			j["net_labels"][(std::string)it.first] = it.second.serialize();
		}
		j["power_symbols"] = json::object();
		for(const auto &it: power_symbols) {
			j["power_symbols"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}
}
