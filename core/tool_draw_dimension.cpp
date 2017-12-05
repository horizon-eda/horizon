#include "tool_draw_dimension.hpp"
#include "imp_interface.hpp"
#include <algorithm>
#include "dimension.hpp"
#include "part.hpp"

namespace horizon {
	ToolDrawDimension::ToolDrawDimension(Core *c, ToolID tid):ToolBase(c, tid) {
		name = "Draw Dim";
	}

	bool ToolDrawDimension::can_begin() {
		return core.r->has_object_type(ObjectType::DIMENSION);
	}

	ToolResponse ToolDrawDimension::begin(const ToolArgs &args) {
		temp = core.r->insert_dimension(UUID::random());
		temp->temp = true;
		temp->p0 = args.coords;
		temp->p1 = args.coords;
		update_tip();
		return ToolResponse();
	}

	void ToolDrawDimension::update_tip() {
		std::stringstream ss;
		ss << "<b>LMB:</b>place ";
		switch(state) {
			case State::P0 :
				ss << "first point";
			break;
			case State::P1 :
				ss << "second point";
			break;
			case State::LABEL :
				ss << "label";
			break;
		}
		ss << " <b>RMB:</b>cancel";
		if(state == State::LABEL) {
			ss << " <b>e:</b>flip <b>m:</b>mode ";
			ss << "<i>";
			switch(temp->mode) {
				case Dimension::Mode::DISTANCE :
					ss << "Distance";
				break;
				case Dimension::Mode::HORIZONTAL :
					ss << "Horizontal";
				break;
				case Dimension::Mode::VERTICAL :
					ss << "Vertical";
				break;
			}
			ss << "</i>";
		}
		imp->tool_bar_set_tip(ss.str());
	}

	ToolResponse ToolDrawDimension::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			switch(state) {
				case State::P0 :
					temp->p0 = args.coords;
					temp->p1 = args.coords;
				break;
				case State::P1 :
					temp->p1 = args.coords;
				break;
				case State::LABEL : {
					temp->label_distance = temp->project(args.coords-temp->p0);
				} break;
			}
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				switch(state) {
					case State::P0 :
						state = State::P1;
					break;
					case State::P1 :
						state = State::LABEL;
					break;
					case State::LABEL :
						temp->temp = false;
						core.r->commit();
						return ToolResponse::end();
					break;
				}
				update_tip();
			}
			else if(args.button == 3) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_e) {
				if(state == State::LABEL) {
					std::swap(temp->p0, temp->p1);
					temp->label_distance *= -1;
				}
			}
			else if(args.key == GDK_KEY_m) {
				switch(temp->mode) {
					case Dimension::Mode::DISTANCE :
						temp->mode = Dimension::Mode::HORIZONTAL;
					break;
					case Dimension::Mode::HORIZONTAL :
						temp->mode = Dimension::Mode::VERTICAL;
					break;
					case Dimension::Mode::VERTICAL :
						temp->mode = Dimension::Mode::DISTANCE;
					break;
				}
				temp->label_distance = temp->project(args.coords-temp->p0);
				update_tip();
			}
			else if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		return ToolResponse();
	}

}
