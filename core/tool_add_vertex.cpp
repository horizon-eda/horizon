#include "tool_add_vertex.hpp"
#include <iostream>
#include "imp_interface.hpp"
#include "polygon.hpp"

namespace horizon {

	ToolAddVertex::ToolAddVertex(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Add vertex";
	}

	bool ToolAddVertex::can_begin() {
		return std::count_if(core.r->selection.begin(), core.r->selection.end(), [](const auto &x){return x.type == ObjectType::POLYGON_EDGE;})==1;
	}

	ToolResponse ToolAddVertex::begin(const ToolArgs &args) {
		std::cout << "tool bend net line\n";

		if(!can_begin())
			return ToolResponse::end();

		int v = 0;
		for(const auto &it: args.selection) {
			if(it.type == ObjectType::POLYGON_EDGE) {
				poly = core.r->get_polygon(it.uuid);
				v = it.vertex;
				break;
			}
		}

		if(!poly)
			return ToolResponse::end();

		auto v_next = (v+1)%poly->vertices.size();
		vertex = &*poly->vertices.insert(poly->vertices.begin()+v_next, args.coords);
		core.r->selection.clear();
		core.r->selection.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, v_next);
		imp->tool_bar_set_tip("<b>LMB:</b>place vertex <b>RMB:</b>cancel");
		return ToolResponse();
	}
	ToolResponse ToolAddVertex::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			vertex->position = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				core.r->commit();
				return ToolResponse::end();
			}
			else if(args.button == 3) {
				core.r->revert();
				core.r->selection.clear();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				core.r->selection.clear();
				return ToolResponse::end();
			}
		}
		return ToolResponse();
	}

}
