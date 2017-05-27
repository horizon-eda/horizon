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
		auto sch = core.c->get_schematic();
		if(tool_id == ToolID::MANAGE_BUSES) {
			r = imp->dialogs.manage_buses(sch->block);
		}
		else if(tool_id == ToolID::ANNOTATE) {
			r = imp->dialogs.annotate(sch);
		}
		else if(tool_id == ToolID::MANAGE_NET_CLASSES) {
			r = imp->dialogs.manage_net_classes(sch->block);
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
