#include "tool_edit_parameter_program.hpp"
#include "core_padstack.hpp"
#include <iostream>

namespace horizon {

	ToolEditParameterProgram::ToolEditParameterProgram(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	bool ToolEditParameterProgram::can_begin() {
		return core.a;
	}

	ToolResponse ToolEditParameterProgram::begin(const ToolArgs &args) {
		bool r;
		auto ps = core.a->get_padstack();
		if(tool_id == ToolID::EDIT_PARAMETER_PROGRAM) {
			r = imp->dialogs.edit_parameter_program(&ps->parameter_program);
		}
		else {
			r = imp->dialogs.edit_parameter_set(&ps->parameter_set);
		}
		if(r) {
			core.r->commit();
		}
		else {
			core.r->revert();
		}
		return ToolResponse::end();
	}
	ToolResponse ToolEditParameterProgram::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
