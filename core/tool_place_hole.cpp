#include "tool_place_hole.hpp"
#include <iostream>
#include "core_padstack.hpp"

namespace horizon {
	
	ToolPlaceHole::ToolPlaceHole(Core *c, ToolID tid):ToolBase(c, tid) {
		name = "Place Hole";
	}
	
	bool ToolPlaceHole::can_begin() {
		return core.r->has_object_type(ObjectType::HOLE);
	}
	
	ToolResponse ToolPlaceHole::begin(const ToolArgs &args) {
		std::cout << "tool place hole\n";

		create_hole(args.coords);
		
		return ToolResponse();
	}

	void ToolPlaceHole::create_hole(const Coordi &c) {
		temp = core.r->insert_hole(UUID::random());
		temp->position = c;
	}

	ToolResponse ToolPlaceHole::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			temp->position = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				holes_placed.push_front(temp);

				create_hole(args.coords);
			}
			else if(args.button == 3) {
				core.r->delete_hole(temp->uuid);
				temp = 0;
				core.r->commit();
				core.r->selection.clear();
				for(auto it: holes_placed) {
					core.r->selection.emplace(it->uuid, ObjectType::HOLE);
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
