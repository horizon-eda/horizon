#include "tool_manage_buses.hpp"
#include "core_schematic.hpp"
#include <iostream>

namespace horizon {

	ToolManageBuses::ToolManageBuses(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Bus Manager";
	}

	bool ToolManageBuses::can_begin() {
		return core.c;
	}

	ToolResponse ToolManageBuses::begin(const ToolArgs &args) {
		bool r;
		if(tool_id == ToolID::MANAGE_BUSES) {
			r = core.r->dialogs.manage_buses();
		}
		else if(tool_id == ToolID::ANNOTATE) {
			r = core.r->dialogs.annotate();
		}
		if(r) {
			core.r->commit();
		}
		else {
			core.r->revert();
		}
		return ToolResponse::end();
	}
	ToolResponse ToolManageBuses::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
