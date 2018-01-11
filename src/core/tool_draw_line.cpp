#include "tool_draw_line.hpp"
#include "imp/imp_interface.hpp"
#include <algorithm>
#include "pool/part.hpp"

namespace horizon {
	
	ToolDrawLine::ToolDrawLine(Core *c, ToolID tid):ToolBase(c, tid) {
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
		update_tip();
		
		return ToolResponse();
	}

	void ToolDrawLine::update_tip() {
		imp->tool_bar_set_tip("<b>LMB:</b>place junction/connect <b>RMB:</b>finish and delete last segment <b>w:</b>line width");
	}

	ToolResponse ToolDrawLine::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			temp_junc->position = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				int64_t last_width = 0;
				if(temp_line)
					last_width = temp_line->width;
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
				temp_line->width = last_width;
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
			if(args.key == GDK_KEY_w) {
				if(temp_line) {
					auto r = imp->dialogs.ask_datum("Enter width", temp_line->width);
					if(r.first) {
						temp_line->width = std::max(r.second, (int64_t)0);
					}
				}
			}
			else if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		return ToolResponse();
	}
	
}
