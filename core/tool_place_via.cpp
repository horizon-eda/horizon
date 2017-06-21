#include "tool_place_via.hpp"
#include <iostream>
#include "core_board.hpp"
#include "imp_interface.hpp"

namespace horizon {

	ToolPlaceVia::ToolPlaceVia(Core *c, ToolID tid): ToolPlaceJunction(c, tid) {
		name = "Place Via";
	}

	bool ToolPlaceVia::can_begin() {
		return core.b;
	}

	bool ToolPlaceVia::begin_attached() {
		bool r;
		UUID template_uuid;
		std::tie(r, template_uuid) = imp->dialogs.select_via_template(core.b->get_board());
		if(!r) {
			return false;
		}
		vt = &core.b->get_board()->via_templates.at(template_uuid);
		imp->tool_bar_set_tip("<b>LMB:</b>place via <b>RMB:</b>delete current via and finish");
		return true;
	}

	void ToolPlaceVia::create_attached() {
		auto uu = UUID::random();
		via = &core.b->get_board()->vias.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, vt)).first->second;
		via->padstack.apply_parameter_set(via->via_template->parameter_set);
		via->junction = temp;
	}

	void ToolPlaceVia::delete_attached() {
		if(via) {
			core.b->get_board()->vias.erase(via->uuid);
		}
	}

	bool ToolPlaceVia::update_attached(const ToolArgs &args) {
		if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(args.target.type == ObjectType::JUNCTION) {
					Junction *j = core.r->get_junction(args.target.path.at(0));
					via->junction = j;
					create_attached();
					return true;
				}
			}
		}
		return false;
	}
}
