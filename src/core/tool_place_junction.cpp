#include "tool_place_junction.hpp"
#include <iostream>
#include "core_schematic.hpp"
#include "core_symbol.hpp"

namespace horizon {
	
	ToolPlaceJunction::ToolPlaceJunction(Core *c, ToolID tid):ToolBase(c, tid) {
	}
	
	bool ToolPlaceJunction::can_begin() {
		return core.r->has_object_type(ObjectType::JUNCTION);
	}

	
	ToolResponse ToolPlaceJunction::begin(const ToolArgs &args) {
		std::cout << "tool place junction\n";
		
		if(!begin_attached()) {
			return ToolResponse::end();
		}

		create_junction(args.coords);
		create_attached();
		
		return ToolResponse();
	}

	void ToolPlaceJunction::create_junction(const Coordi &c) {
		temp = core.r->insert_junction(UUID::random());
		temp->temp = true;
		temp->position = c;
	}

	ToolResponse ToolPlaceJunction::update(const ToolArgs &args) {
		if(update_attached(args)) {
			return ToolResponse();
		}

		if(args.type == ToolEventType::MOVE) {
			temp->position = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(core.c) {
					for(auto it: core.c->get_net_lines()) {
						if(it->coord_on_line(temp->position)) {
							std::cout << "on line" << std::endl;
							if(!check_line(it))
								return ToolResponse();

							core.c->get_sheet()->split_line_net(it, temp);
							break;
						}
					}
				}
				temp->temp = false;
				junctions_placed.push_front(temp);

				create_junction(args.coords);
				create_attached();
			}
			else if(args.button == 3) {
				delete_attached();
				core.r->delete_junction(temp->uuid);
				temp = 0;
				core.r->commit();
				core.r->selection.clear();
				for(auto it: junctions_placed) {
					core.r->selection.emplace(it->uuid, ObjectType::JUNCTION);
				}
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		return ToolResponse();
	}
	
}
