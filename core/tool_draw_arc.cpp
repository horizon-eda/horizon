#include "tool_draw_arc.hpp"
#include "imp_interface.hpp"
#include <iostream>

namespace horizon {
	
	ToolDrawArc::ToolDrawArc(Core *c, ToolID tid):ToolBase(c, tid) {
		name = "Draw Arc";
	}
	
	bool ToolDrawArc::can_begin() {
		return core.r->has_object_type(ObjectType::ARC);
	}

	Junction *ToolDrawArc::make_junction(const Coordi &coords) {
		Junction *ju;
		ju = core.r->insert_junction(UUID::random());
		ju->temp = true;
		ju->position = coords;
		return ju;
	}
	
	ToolResponse ToolDrawArc::begin(const ToolArgs &args) {
		std::cout << "tool draw arc\n";
		
		temp_junc = make_junction(args.coords);
		temp_arc = nullptr;
		from_junc = nullptr;
		to_junc = nullptr;
		state = DrawArcState::FROM;
		update_tip();
		return ToolResponse();
	}

	void ToolDrawArc::update_tip() {
		std::stringstream ss;
		ss << "<b>LMB:</b>";
		if(state == DrawArcState::FROM) {
			ss << "place from junction";
		}
		else if(state == DrawArcState::TO) {
			ss << "place to junction";
		}
		else if(state == DrawArcState::CENTER) {
			ss << "place center junction";
		}
		ss << " <b>RMB:</b>cancel <b>e:</b>reverse arc direction";
		imp->tool_bar_set_tip(ss.str());
	}

	ToolResponse ToolDrawArc::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			temp_junc->position = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(state == DrawArcState::FROM) {
					if(args.target.type == ObjectType::JUNCTION) {
						from_junc = core.r->get_junction(args.target.path.at(0));
					}
					else {
						temp_junc->temp = false;
						from_junc = temp_junc;
						temp_junc = make_junction(args.coords);
					}
					state = DrawArcState::TO;
				}
				else if(state == DrawArcState::TO) {
					if(args.target.type == ObjectType::JUNCTION) {
						to_junc = core.r->get_junction(args.target.path.at(0));
					}
					else {
						temp_junc->temp = false;
						to_junc = temp_junc;
						temp_junc = make_junction(args.coords);
					}
					temp_arc = core.r->insert_arc(UUID::random());
					temp_arc->from = from_junc;
					temp_arc->to = to_junc;
					temp_arc->center = temp_junc;
					state = DrawArcState::CENTER;
				}
				else if(state == DrawArcState::CENTER) {
					if(args.target.type == ObjectType::JUNCTION) {
						temp_arc->center = core.r->get_junction(args.target.path.at(0));
						core.r->delete_junction(temp_junc->uuid);
						temp_junc = nullptr;
					}
					else {
						temp_junc->temp = false;
						temp_arc->center = temp_junc;
					}
					core.r->commit();
					return ToolResponse::end();
				}
			}
			else if(args.button == 3) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
			else if(args.key == GDK_KEY_e) {
				if(temp_arc) {
					temp_arc->reverse();
				}
			}
		}
		update_tip();
		return ToolResponse();
	}
	
}
