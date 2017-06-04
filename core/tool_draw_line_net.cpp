#include "tool_draw_line_net.hpp"
#include <iostream>
#include "core_schematic.hpp"
#include "imp_interface.hpp"

namespace horizon {
	
	ToolDrawLineNet::ToolDrawLineNet(Core *c, ToolID tid):ToolBase(c, tid) {
		name = "Draw Net Line";
	}
	
	bool ToolDrawLineNet::can_begin() {
		return core.c;
	}

	
	ToolResponse ToolDrawLineNet::begin(const ToolArgs &args) {
		std::cout << "tool draw net line junction\n";
		
		temp_junc_head = core.c->insert_junction(UUID::random());
		temp_junc_head->temp = true;
		temp_junc_head->position = args.coords;
		core.c->selection.clear();

		update_tip();
		return ToolResponse();
	}

	void ToolDrawLineNet::move_temp_junc(const Coordi &c) {
		temp_junc_head->position = c;
		if(temp_line_head){
			auto tail_pos = temp_line_mid->from.get_position();
			auto head_pos = temp_junc_head->position;

			if(bend_mode == BendMode::XY) {
				temp_junc_mid->position = {head_pos.x, tail_pos.y};
			}
			else if(bend_mode == BendMode::YX) {
				temp_junc_mid->position = {tail_pos.x, head_pos.y};
			}
			else if(bend_mode == BendMode::ARB) {
				temp_junc_mid->position = tail_pos;
			}
		}
	}

	int ToolDrawLineNet::merge_nets(Net *net, Net *into) {
		if(net->is_bussed || into->is_bussed) {
			if(net->is_power || into->is_power) {
				imp->tool_bar_flash("can't merge power and bussed net");
				return 1; //don't merge power and bus
			}
			else if(net->is_bussed && into->is_bussed) {
				imp->tool_bar_flash("can't merge bussed nets");
				return 1; //can't merge bussed nets
			}
			else if(!net->is_bussed && into->is_bussed) {
				//merging non-bussed net into bussed net is fine
			}
			else if(net->is_bussed && !into->is_bussed) {
				std::swap(net, into);
			}

		}
		else if(net->is_power || into->is_power) {
			if(net->is_power && into->is_power) {
				imp->tool_bar_flash("can't merge power nets");
				return 1;
			}
			else if(!net->is_power && into->is_power) {
				//merging non-power net into power net is fine
			}
			else if(net->is_power && !into->is_power) {
				std::swap(net, into);
			}
		}
		else if(net->is_named() && into->is_named()) {
			int r = imp->dialogs.ask_net_merge(net, into);
			if(r==1) {
				//nop, nets are already as we want them to
			}
			else if(r == 2) {
				std::swap(net, into);
			}
			else {
				//don't merge nets
				return 1;
			}
		}
		else if(net->is_named()) {
			std::swap(net, into);
		}

		imp->tool_bar_flash("merged net \""+net->name+"\" into net \"" + into->name +"\"");
		core.c->get_schematic()->block->merge_nets(net, into); //net will be erased
		core.c->get_schematic()->expand(true); //be careful

		std::cout << "merging nets" << std::endl;
		return 0; //merged, 1: error
	}

	Junction *ToolDrawLineNet::make_temp_junc(const Coordi &c) {
		Junction *j = core.r->insert_junction(UUID::random());
		j->position = c;
		j->temp = true;
		return j;
	}

	ToolResponse ToolDrawLineNet::end() {
		return ToolResponse::next(ToolID::DRAW_NET);
	}

	void ToolDrawLineNet::update_tip() {
		std::stringstream ss;
		ss << "<b>LMB:</b>place junction/connect <b>RMB:</b>finish and delete last segment <b>space:</b>place junction <b>⏎:</b>finish <b>/:</b>line posture <b>a:</b>arbitrary angle   <i>";
		if(temp_line_head) {
			if(temp_line_head->net) {
				if(temp_line_head->net->name.size()) {
					ss << "drawing net \"" << temp_line_head->net->name << "\"";
				}
				else {
					ss << "drawing unnamed net";
				}
			}
			else if(temp_line_head->bus) {
				ss << "drawing bus \"" << temp_line_head->bus->name << "\"";
			}
			else {
				ss << "drawing no net";
			}

		}
		else {
			ss << "select starting point";
		}
		ss<<"</i>";
		imp->tool_bar_set_tip(ss.str());
	}

	ToolResponse ToolDrawLineNet::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			move_temp_junc(args.coords);
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				Junction *ju = nullptr;
				SchematicSymbol *sym = nullptr;
				SymbolPin *pin = nullptr;
				BusRipper *rip = nullptr;
				Net *net = nullptr;
				Bus *bus = nullptr;
				if(!temp_line_head) { //no line yet, first click
					if(args.target.type == ObjectType::JUNCTION) {
						ju = core.r->get_junction(args.target.path.at(0));
						net = ju->net;
						bus = ju->bus;
					}
					else if(args.target.type == ObjectType::SYMBOL_PIN) {
						sym = core.c->get_schematic_symbol(args.target.path.at(0));
						pin = &sym->symbol.pins.at(args.target.path.at(1));
						UUIDPath<2> connpath(sym->gate->uuid, args.target.path.at(1));

						if(sym->component->connections.count(connpath)) { //pin is connected
							Connection &conn = sym->component->connections.at(connpath);
							net = conn.net;
						}
						else { //pin needs net
							net = core.c->get_schematic()->block->insert_net();
							sym->component->connections.emplace(connpath, net);
						}
					}
					else if(args.target.type == ObjectType::BUS_RIPPER) {
						rip = &core.c->get_sheet()->bus_rippers.at(args.target.path.at(0));
						net = rip->bus_member->net;
					}
					else { //unknown or no target
						ju = make_temp_junc(args.coords);
						for(auto it: core.c->get_net_lines()) {
							if(it->coord_on_line(temp_junc_head->position)) {
								if(it != temp_line_head && it != temp_line_mid) {
									bool is_temp_line = false;
									for(const auto &it_ft: {it->from, it->to}) {
										if(it_ft.is_junc()) {
											if(it_ft.junc->temp)
												is_temp_line = true;
										}
									}
									if(!is_temp_line) {
										imp->tool_bar_flash("split net line");
										auto li = core.c->get_sheet()->split_line_net(it, ju);
										net = li->net;
										bus = li->bus;
										ju->net = li->net;
										ju->bus = li->bus;
										ju->connection_count = 3;
										break;
									}
								}
							}
						}

					}

					temp_junc_mid = make_temp_junc(args.coords);
					temp_junc_mid->net = net;
					temp_junc_mid->bus = bus;
					temp_junc_head->net = net;
					temp_junc_head->bus = bus;

				}
				else { //already have net line
					if(args.target.type == ObjectType::JUNCTION) {
						ju = core.r->get_junction(args.target.path.at(0));
						if(temp_line_head->bus || ju->bus) { //either is bus
							if(temp_line_head->net || ju->net) {
								imp->tool_bar_flash("can't connect bus and net");
								return ToolResponse(); //bus-net illegal
							}

							if(temp_line_head->bus && ju->bus) { //both are bus
								if(temp_line_head->bus != ju->bus) { //not the same bus
									imp->tool_bar_flash("can't connect different buses");
									return ToolResponse(); //illegal
								}
								else  {
									temp_line_head->to.connect(ju);
									core.r->delete_junction(temp_junc_head->uuid);
									core.r->commit();
									return end();
								}
							}
							if((temp_line_head->bus && !ju->bus) || (!temp_line_head->bus && ju->bus)) {
								temp_line_head->to.connect(ju);
								core.r->delete_junction(temp_junc_head->uuid);
								core.r->commit();
								return end();
							}
							assert(false); //dont go here
						}
						else if(temp_line_head->net && ju->net && ju->net.uuid != temp_line_head->net->uuid) { //both have nets, need to merge
							if(merge_nets(ju->net, temp_line_head->net)) {
								return ToolResponse();
							}
							else {
								temp_line_head->to.connect(ju);
								core.r->delete_junction(temp_junc_head->uuid);
								core.r->commit();
								return end();
							}
						}
						else  { //just connect it
							temp_line_head->to.connect(ju);
							core.r->delete_junction(temp_junc_head->uuid);
							core.r->commit();
							return end();
						}
					}
					else if(args.target.type == ObjectType::SYMBOL_PIN) {
						sym = core.c->get_schematic_symbol(args.target.path.at(0));
						pin = &sym->symbol.pins.at(args.target.path.at(1));
						UUIDPath<2> connpath(sym->gate->uuid, args.target.path.at(1));
						if(temp_line_head->bus) { //can't connect bus to pin
							return ToolResponse();
						}
						if(sym->component->connections.count(connpath)) { //pin is connected
							Connection &conn = sym->component->connections.at(connpath);
							if(temp_line_head->net && (temp_line_head->net->uuid != conn.net->uuid)) { //has net
								if(merge_nets(conn.net, temp_line_head->net)) {
									return ToolResponse();
								}
							}
						}
						else { //pin needs net
							if(temp_junc_head->net) {
								net = temp_junc_head->net;
							}
							else {
								net = core.c->get_schematic()->block->insert_net();
							}
							sym->component->connections.emplace(connpath, net);
						}
						temp_line_head->to.connect(sym, pin);
						core.r->delete_junction(temp_junc_head->uuid);
						core.r->commit();
						return end();
					}
					else if(args.target.type == ObjectType::BUS_RIPPER) {
						rip = &core.c->get_sheet()->bus_rippers.at(args.target.path.at(0));
						net = rip->bus_member->net;
						if(temp_line_head->bus) { //can't connect bus to bus ripper
							return ToolResponse();
						}
						if(temp_line_head->net && (temp_line_head->net->uuid != net->uuid)) { //has net
							if(merge_nets(net, temp_line_head->net)) {
								return ToolResponse();
							}
						}

						temp_line_head->to.connect(rip);
						core.r->delete_junction(temp_junc_head->uuid);
						core.r->commit();
						return end();
					}
					else { //unknown or no target
						ju = temp_junc_head;
						for(auto it: core.c->get_net_lines()) {
							if(it->coord_on_line(temp_junc_head->position)) {
								if(it != temp_line_head && it != temp_line_mid) {
									bool is_temp_line = false;
									for(const auto &it_ft: {it->from, it->to}) {
										if(it_ft.is_junc()) {
											if(it_ft.junc->temp)
												is_temp_line = true;
										}
									}
									if(!is_temp_line) {
										imp->tool_bar_flash("split net line");
										net = it->net;
										bus = it->bus;

										if(temp_line_head->bus || it->bus) { //either is bus
											if(temp_line_head->net || it->net) {
												imp->tool_bar_flash("can't connect bus and net");
												return ToolResponse(); //bus-net illegal
											}

											if(temp_line_head->bus && it->bus) { //both are bus
												if(temp_line_head->bus != it->bus) { //not the same bus
													imp->tool_bar_flash("can't connect different buses");
													return ToolResponse(); //illegal
												}
												else  {
													core.c->get_sheet()->split_line_net(it, ju);
													core.r->commit();
													return end();
												}
											}
											if((temp_line_head->bus && !ju->bus) || (!temp_line_head->bus && ju->bus)) {
												core.c->get_sheet()->split_line_net(it, ju);
												core.r->commit();
												return end();
											}
											assert(false); //dont go here
										}
										else if(temp_line_head->net && it->net && it->net.uuid != temp_line_head->net->uuid) { //both have nets, need to merge
											if(merge_nets(it->net, temp_line_head->net)) {
												return ToolResponse();
											}
											else {
												core.c->get_sheet()->split_line_net(it, ju);
												core.r->commit();
												return end();
											}
										}
										else  { //just connect it
											core.c->get_sheet()->split_line_net(it, ju);
											core.r->commit();
											return end();
										}
										break;
									}
								}
							}
						}
						net = ju->net;
						bus = ju->bus;
						temp_junc_mid = make_temp_junc(args.coords);
						temp_junc_mid->net = net;
						temp_junc_mid->bus= bus;

						temp_junc_head = make_temp_junc(args.coords);
						temp_junc_head->net = net;
						temp_junc_head->bus= bus;
					}
				}
				temp_line_mid = core.c->insert_line_net(UUID::random());
				temp_line_mid->net = net;
				temp_line_mid->bus = bus;
				if(ju) {
					temp_line_mid->from.connect(ju);
				}
				else if(sym){
					temp_line_mid->from.connect(sym, pin);
				}
				else if(rip){
					temp_line_mid->from.connect(rip);
				}
				else {
					assert(false);
				}
				temp_line_mid->to.connect(temp_junc_mid);

				temp_line_head = core.c->insert_line_net(UUID::random());
				temp_line_head->net = net;
				temp_line_head->bus = bus;
				temp_line_head->from.connect(temp_junc_mid);
				temp_line_head->to.connect(temp_junc_head);

			}
			else if(args.button == 3) {
				if(temp_line_head) {
					core.c->delete_line_net(temp_line_head->uuid);
					core.c->delete_line_net(temp_line_mid->uuid);
					core.r->delete_junction(temp_junc_mid->uuid);
				}
				core.r->delete_junction(temp_junc_head->uuid);

				core.c->commit();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.c->revert();
				return ToolResponse::end();
			}
			else if(args.key == GDK_KEY_space) {
				Junction *ju = temp_junc_head;
				temp_junc_mid = make_temp_junc(args.coords);
				temp_junc_mid->net = ju->net;
				temp_junc_mid->bus= ju->bus;

				temp_junc_head = make_temp_junc(args.coords);
				temp_junc_head->net = ju->net;
				temp_junc_head->bus= ju->bus;

				temp_line_mid = core.c->insert_line_net(UUID::random());
				temp_line_mid->net = ju->net;
				temp_line_mid->bus = ju->bus;
				temp_line_mid->from.connect(ju);
				temp_line_mid->to.connect(temp_junc_mid);

				temp_line_head = core.c->insert_line_net(UUID::random());
				temp_line_head->net = ju->net;
				temp_line_head->bus = ju->bus;
				temp_line_head->from.connect(temp_junc_mid);
				temp_line_head->to.connect(temp_junc_head);

			}
			else if(args.key == GDK_KEY_Return) {
				core.c->commit();
				return ToolResponse::end();
			}
			else if(args.key == GDK_KEY_slash) {
				switch(bend_mode) {
					case BendMode::XY: bend_mode=BendMode::YX; break;
					case BendMode::YX: bend_mode=BendMode::XY; break;
					default: bend_mode=BendMode::XY;
				}
				move_temp_junc(args.coords);
			}
			else if(args.key == GDK_KEY_a) {
				bend_mode = BendMode::ARB;
				move_temp_junc(args.coords);
			}
		}
		update_tip();
		return ToolResponse();
	}
}
