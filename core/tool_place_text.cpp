#include "tool_place_text.hpp"
#include <iostream>

namespace horizon {
	
	ToolPlaceText::ToolPlaceText(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Place Text";
	}
	
	bool ToolPlaceText::can_begin() {
		return core.r->has_object_type(ObjectType::TEXT);
	}
	
	ToolResponse ToolPlaceText::begin(const ToolArgs &args) {
		std::cout << "tool place text\n";
		
		temp = core.r->insert_text(UUID::random());
		temp->text = "TEXT";
		temp->layer = args.work_layer;
		temp->placement.shift = args.coords;
		
		return ToolResponse();
	}
	ToolResponse ToolPlaceText::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			temp->placement.shift = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				texts_placed.push_front(temp);
				temp = core.r->insert_text(UUID::random());
				temp->text = "TEXT";
				temp->layer = args.work_layer;
				temp->placement.shift = args.coords;
			}
			else if(args.button == 3) {
				core.r->delete_text(temp->uuid);
				core.r->selection.clear();
				for(auto it: texts_placed) {
					core.r->selection.emplace(it->uuid, ObjectType::TEXT);
				}
				core.r->commit();
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
