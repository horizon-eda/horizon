#include "tool_disconnect.hpp"
#include <iostream>
#include "core_schematic.hpp"

namespace horizon {

	ToolDisconnect::ToolDisconnect(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	bool ToolDisconnect::can_begin() {
		for(const auto &it : core.r->selection) {
			if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				return true;
			}
		}
		return false;
	}

	ToolResponse ToolDisconnect::begin(const ToolArgs &args) {
		std::cout << "tool disconnect\n";
		for(const auto &it : args.selection) {
			if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				core.c->get_schematic()->disconnect_symbol(core.c->get_sheet(), core.c->get_schematic_symbol(it.uuid));
			}
		}
		core.c->commit();
		return ToolResponse::end();
	}
	ToolResponse ToolDisconnect::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
