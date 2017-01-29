#include "tool_place_via.hpp"
#include <iostream>
#include "core_board.hpp"

namespace horizon {

	ToolPlaceVia::ToolPlaceVia(Core *c, ToolID tid): ToolPlaceJunction(c, tid) {
		name = "Place Via";
	}

	bool ToolPlaceVia::can_begin() {
		return core.b;
	}

	bool ToolPlaceVia::begin_attached() {
		bool r;
		UUID padstack_uuid;
		std::tie(r, padstack_uuid) = core.r->dialogs.select_via_padstack(core.b->get_via_padstack_provider());
		if(!r) {
			return false;
		}
		padstack = core.b->get_via_padstack_provider()->get_padstack(padstack_uuid);
		return true;
	}

	void ToolPlaceVia::create_attached() {
		auto uu = UUID::random();
		via = &core.b->get_board()->vias.emplace(uu, Via(uu, padstack)).first->second;
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
