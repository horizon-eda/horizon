#include "tool_draw_line.hpp"
#include "core_schematic.hpp"
#include "core_symbol.hpp"
#include <iostream>

namespace horizon {
	
	ToolDrawLine::ToolDrawLine(Core *c, ToolID tid):ToolBase(c, tid) {
		name = "Draw Line";
	}
	
	bool ToolDrawLine::can_begin() {
		return core.r->has_object_type(ObjectType::LINE);
	}

	
	ToolResponse ToolDrawLine::begin(const ToolArgs &args) {
		std::cout << "tool draw line junction\n";
		
		temp_junc = core.r->insert_junction(UUID::random());
		temp_junc->temp = true;
		temp_junc->position = args.coords;
		temp_line = nullptr;
		
		return ToolResponse();
	}
	ToolResponse ToolDrawLine::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			temp_junc->position = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(args.target.type == ObjectType::JUNCTION) {
					if(temp_line != nullptr) {
						temp_line->to = core.r->get_junction(args.target.path.at(0));
					}
					temp_line = core.r->insert_line(UUID::random());
					temp_line->from = core.r->get_junction(args.target.path.at(0));
				}
				else {
					Junction *last = temp_junc;
					temp_junc->temp = false;
					temp_junc = core.r->insert_junction(UUID::random());
					temp_junc->temp = true;
					temp_junc->position = args.coords;

					temp_line = core.r->insert_line(UUID::random());
					temp_line->from = last;
				}
				temp_line->layer = args.work_layer;
				temp_line->to = temp_junc;
			}
			else if(args.button == 3) {
				if(temp_line) {
					core.r->delete_line(temp_line->uuid);
					temp_line = nullptr;
				}
				core.r->delete_junction(temp_junc->uuid);
				temp_junc = nullptr;
				core.r->commit();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::LAYER_CHANGE) {
			if(temp_line)
				temp_line->layer = args.work_layer;
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
