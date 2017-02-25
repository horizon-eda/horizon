#include "tool_place_bus_ripper.hpp"
#include <iostream>
#include "core_schematic.hpp"

namespace horizon {

	ToolPlaceBusRipper::ToolPlaceBusRipper(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Place Bus Ripper";
	}

	bool ToolPlaceBusRipper::can_begin() {
		return core.c;
	}


	ToolResponse ToolPlaceBusRipper::begin(const ToolArgs &args) {
		std::cout << "tool place bus ripper\n";

		core.r->tool_bar_set_tip("<b>LMB:</b>place <b>RMB:</b>finish");
		return ToolResponse();
	}

	ToolResponse ToolPlaceBusRipper::update(const ToolArgs &args) {
		if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(args.target.type == ObjectType::JUNCTION) {
					Junction *j = core.c->get_junction(args.target.path.at(0));
					if(!j->bus) {
						core.r->tool_bar_flash("junction doesn't have bus");
						return ToolResponse();
					}
					Bus *bus = j->bus;
					bool r;
					UUID bus_member_uuid;

					std::tie(r, bus_member_uuid) = core.r->dialogs.select_bus_member(bus->uuid);
					if(!r)
						return ToolResponse();
					Bus::Member *bus_member = &bus->members.at(bus_member_uuid);

					auto uu = UUID::random();
					BusRipper *rip = &core.c->get_sheet()->bus_rippers.emplace(uu, uu).first->second;
					rip->bus = bus;
					rip->bus_member = bus_member;
					rip->junction = j;
				}
				else if(args.target.type == ObjectType::INVALID) {
					for(auto it: core.c->get_net_lines()) {
						if(it->coord_on_line(args.coords)) {
							std::cout << "on line" << std::endl;
							if(!it->bus) {
								core.r->tool_bar_flash("line doesn't have bus");
								return ToolResponse();
							}
							Bus *bus = it->bus;
							bool r;
							UUID bus_member_uuid;

							std::tie(r, bus_member_uuid) = core.r->dialogs.select_bus_member(bus->uuid);
							if(!r)
								return ToolResponse();
							Bus::Member *bus_member = &bus->members.at(bus_member_uuid);

							Junction *j = core.c->insert_junction(UUID::random());
							j->position = args.coords;
							j->bus = bus;

							core.c->get_sheet()->split_line_net(it, j);

							auto uu = UUID::random();
							BusRipper *rip = &core.c->get_sheet()->bus_rippers.emplace(uu, uu).first->second;
							rip->bus = bus;
							rip->bus_member = bus_member;
							rip->junction = j;


							return ToolResponse();
						}
					}

					core.r->tool_bar_flash("can only place bus ripper on buses");
				}
				else {
					core.r->tool_bar_flash("can only place bus ripper on buses");
				}
			}
			else if(args.button == 3) {
				//core.c->get_sheet()->power_symbols.erase(temp->uuid);
				//temp = 0;
				core.c->commit();
				core.c->selection.clear();
				//for(auto it: symbols_placed) {
				//	core.c->selection.emplace(it->uuid, ObjectType::POWER_SYMBOL);
				//}
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.c->revert();
				return ToolResponse::end();
			}
		}
		return ToolResponse();
	}

}
