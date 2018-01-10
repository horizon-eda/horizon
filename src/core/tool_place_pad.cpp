#include "tool_place_pad.hpp"
#include <iostream>
#include "core_package.hpp"
#include "imp_interface.hpp"

namespace horizon {

	ToolPlacePad::ToolPlacePad(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	bool ToolPlacePad::can_begin() {
		return core.k;
	}

	ToolResponse ToolPlacePad::begin(const ToolArgs &args) {
		std::cout << "tool add comp\n";
		bool r;
		UUID padstack_uuid;
		std::tie(r, padstack_uuid) = imp->dialogs.select_padstack(core.r->m_pool, core.k->get_package()->uuid);
		if(!r) {
			return ToolResponse::end();
		}

		padstack = core.r->m_pool->get_padstack(padstack_uuid);
		create_pad(args.coords);

		imp->tool_bar_set_tip("<b>LMB:</b>place pad <b>RMB:</b>delete current pad and finish");
		return ToolResponse();
	}

	void ToolPlacePad::create_pad(const Coordi &pos) {
		Package *pkg = core.k->get_package();
		auto uu = UUID::random();
		temp = &pkg->pads.emplace(uu, Pad(uu, padstack)).first->second;
		temp->placement.shift = pos;
	}

	ToolResponse ToolPlacePad::update(const ToolArgs &args) {

		if(args.type == ToolEventType::MOVE) {
			temp->placement.shift = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {


				create_pad(args.coords);
			}
			else if(args.button == 3) {
				core.k->get_package()->pads.erase(temp->uuid);
				temp = 0;
				core.r->commit();
				core.r->selection.clear();
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
