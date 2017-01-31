#include "tool_draw_line_net.hpp"
#include <iostream>
#include "core_schematic.hpp"

namespace horizon {
	
	ToolDrawLineNet::ToolDrawLineNet(Core *c, ToolID tid):ToolBase(c, tid) {
		name = "Draw Net Line";
	}
	
	bool ToolDrawLineNet::can_begin() {
		return core.c;
	}

	
	ToolResponse ToolDrawLineNet::begin(const ToolArgs &args) {
		std::cout << "tool draw net line junction\n";
		
		temp_junc = core.c->insert_junction(UUID::random());
		temp_junc->temp = true;
		temp_junc->position = args.coords;
		temp_line = nullptr;
		core.c->selection.clear();
		
		return ToolResponse();
	}

	void ToolDrawLineNet::move_temp_junc(const Coordi &c) {
		if(!temp_line) {
			temp_junc->position = c;
		}
		else {
			switch(restrict_mode) {
				case Restrict::NONE :
					temp_junc->position = c;
				break;

				case Restrict::X :
					temp_junc->position = {c.x, temp_line->from.get_position().y};
				break;
				case Restrict::Y :
					temp_junc->position = {temp_line->from.get_position().x, c.y};
				break;
			}
		}
	}

	int ToolDrawLineNet::merge_nets(Net *net, Net *into) {
		//fixme: mergeing bussed power nets :|
		if(net->is_bussed || into->is_bussed) {
			if(net->is_bussed && into->is_bussed) {
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
			int r = core.r->dialogs.ask_net_merge(net, into);
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
		core.c->get_schematic()->block->merge_nets(net, into); //net will be erased
		core.c->get_schematic()->expand(true); //be careful

		std::cout << "merging nets" << std::endl;
		return 0;
	}

	ToolResponse ToolDrawLineNet::end(bool delete_junction) {
		temp_junc->temp = false;
		if(delete_junction)
			core.c->delete_junction(temp_junc->uuid);
		core.c->commit();
		return ToolResponse::end();
	}

	ToolResponse ToolDrawLineNet::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			move_temp_junc(args.coords);
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(args.target.type == ObjectType::JUNCTION) {
					uuid_ptr<Junction> j = core.c->get_junction(args.target.path.at(0));
					if(temp_line != nullptr) {
						UUID tlu = temp_line->uuid;
						if(temp_line->bus || j->bus) { //either is bus
							if(temp_line->net || j->net)
								return ToolResponse(); //bus-net illegal

							if(temp_line->bus && j->bus) { //both are bus
								if(temp_line->bus != j->bus) //not the same bus
									return ToolResponse(); //illegal
							}
							else if(temp_line->bus && !j->bus) {
								j->bus = temp_line->bus;
							}
							else if(!temp_line->bus && j->bus) {
								temp_line->bus = j->bus;
							}
						}
						if(temp_line->net && j->net && j->net.uuid != temp_line->net->uuid) {
							if(merge_nets(j->net, temp_line->net)) {
								return ToolResponse();
							}
						}
						if(j->net) {
							temp_line->net = j->net;
						}
						else {
							j->net = temp_line->net;
						}
						temp_line->to.connect(j);
						Sheet *sh = core.c->get_sheet();
						assert(sh->junctions.count(j.uuid));
						assert(sh->net_lines.count(tlu));

						return end();

					}

					temp_line = core.c->insert_line_net(UUID::random());
					temp_junc->net = j->net;
					temp_junc->bus = j->bus;
					temp_line->from.connect(j);
					temp_line->to.connect(temp_junc);
					temp_line->net = j->net;
					temp_line->bus = j->bus;

				}
				else if(args.target.type == ObjectType::SYMBOL_PIN) {
					SchematicSymbol *sym = core.c->get_schematic_symbol(args.target.path.at(0));
					UUIDPath<2> connpath(sym->gate->uuid, args.target.path.at(1));

					if(sym->component->connections.count(connpath)) {
						Connection &conn = sym->component->connections.at(connpath);
						if(temp_line != nullptr) {
							if(temp_line->bus) {
								return ToolResponse();
							}
							if(temp_line->net && (temp_line->net->uuid != conn.net->uuid)) {
								if(merge_nets(conn.net, temp_line->net)) {
									return ToolResponse();
								}
							}
							temp_line->to.connect(sym, &sym->pool_symbol->pins.at(args.target.path.at(1)));
							return end();
						}
						else {
							temp_line = core.c->insert_line_net(UUID::random());
							temp_line->from.connect(sym, &sym->pool_symbol->pins.at(args.target.path.at(1)));
							temp_line->to.connect(temp_junc);
							temp_junc->net = conn.net;
							temp_line->net = conn.net;
						}
					}
					else {
						Net *net = nullptr;
						bool have_line = false;
						if(temp_line != nullptr) { //already have line
							if(temp_line->bus) {
								return ToolResponse();
							}
							if(!temp_line->net) { //temp line has no net, create one
								net = core.c->get_schematic()->block->insert_net();
								temp_line->net = net;
							}
							else { //temp line has net
								net = temp_line->net;
							}
							sym->component->connections.emplace(connpath, static_cast<Net*>(temp_line->net));
							//terminate line
							temp_line->to.connect(sym, &sym->pool_symbol->pins.at(args.target.path.at(1)));
							have_line = true;
						}
						//create new line and net
						if(!net) {
							net = core.c->get_schematic()->block->insert_net();
							sym->component->connections.emplace(connpath, net);
						}
						if(have_line) {
							return end();
						}

						temp_line = core.c->insert_line_net(UUID::random());
						temp_line->from.connect(sym, &sym->pool_symbol->pins.at(args.target.path.at(1)));
						temp_line->to.connect(temp_junc);
						temp_junc->net = net;
						temp_line->net = net;

					}

				}
				else if(args.target.type == ObjectType::BUS_RIPPER) {
					BusRipper *ripper = &core.c->get_sheet()->bus_rippers.at(args.target.path.at(0));
					if(temp_line != nullptr) {
						if(temp_line->bus) {
							return ToolResponse();
						}
						if(temp_line->net && (temp_line->net->uuid != ripper->bus_member->net->uuid)) {
							if(merge_nets(ripper->bus_member->net, temp_line->net)) {
								return ToolResponse();
							}
						}
						temp_line->to.connect(ripper);
						return end();
					}
					temp_line = core.c->insert_line_net(UUID::random());
					temp_line->from.connect(ripper);
					temp_line->to.connect(temp_junc);
					temp_junc->net = ripper->bus_member->net;
					temp_line->net = ripper->bus_member->net;
				}

				else {
					Junction *last = temp_junc;


					for(auto it: core.c->get_net_lines()) {
						if(it->coord_on_line(temp_junc->position)) {
							if(it != temp_line) {
								bool is_temp_line = false;
								for(const auto &it_ft: {it->from, it->to}) {
									if(it_ft.is_junc()) {
										if(it_ft.junc->temp)
											is_temp_line = true;
									}
								}
								if(!is_temp_line) {
									std::cout << "on line" << (std::string)it->uuid << std::endl;
									if(temp_junc->bus || it->bus) { //either is bus
										if(temp_junc->net || it->net)
											return ToolResponse(); //bus-net illegal

										if(temp_junc->bus && it->bus) { //both are bus
											if(temp_junc->bus != it->bus) //not the same bus
												return ToolResponse(); //illegal
										}
										else if(temp_junc->bus && !it->bus) {
											it->bus = temp_junc->bus;
										}
										else if(!temp_junc->bus && it->bus) {
											temp_junc->bus = it->bus;
										}
									}
									if(temp_junc->net && it->net && it->net.uuid != temp_junc->net->uuid) {
										if(merge_nets(it->net, temp_junc->net)) {
											return ToolResponse();
										}
									}
									auto li = core.c->get_sheet()->split_line_net(it, temp_junc);
									temp_junc->net = li->net;
									temp_junc->bus = li->bus;

									if(temp_line)
										return end(false);

									break;
								}
							}
						}
					}
					temp_junc = core.c->insert_junction(UUID::random());
					temp_junc->temp = true;
					temp_junc->position = args.coords;
					if(last && temp_line) {
						temp_line->net = last->net;
						temp_line->bus = last->bus;
					}
					if(temp_line) {
						temp_junc->net = temp_line->net;
						temp_junc->bus = temp_line->bus;
					}
					if(last) {
						temp_junc->net = last->net;
						temp_junc->bus = last->bus;
					}
					
					temp_line = core.c->insert_line_net(UUID::random());
					temp_line->from.connect(last);
					temp_line->to.connect(temp_junc);
					temp_line->net = last->net;
					temp_line->bus = last->bus;

					if(restrict_mode == Restrict::X) {
						restrict_mode = Restrict::Y;
					}
					else if(restrict_mode == Restrict::Y) {
						restrict_mode = Restrict::X;
					}
				}
			}
			else if(args.button == 3) {
				if(temp_line) {
					core.c->delete_line_net(temp_line->uuid);
					temp_line = nullptr;
				}
				core.c->delete_junction(temp_junc->uuid);
				temp_junc = nullptr;
				core.c->commit();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.c->revert();
				return ToolResponse::end();
			}
			if(args.key == GDK_KEY_slash) {
				if(restrict_mode == Restrict::NONE) {
					restrict_mode = Restrict::X;
				}
				else if(restrict_mode == Restrict::X) {
					restrict_mode = Restrict::Y;
				}
				else if(restrict_mode == Restrict::Y) {
					restrict_mode = Restrict::NONE;
				}
				move_temp_junc(args.coords);
			}
		}
		return ToolResponse();
	}
	
}
