#include "tool_edit_shape.hpp"
#include "core_padstack.hpp"
#include "imp_interface.hpp"
#include <iostream>

namespace horizon {

	ToolEditShape::ToolEditShape(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	bool ToolEditShape::can_begin() {
		if(!core.a)
			return false;
		return std::count_if(core.r->selection.begin(), core.r->selection.end(), [](const auto &x){return x.type == ObjectType::SHAPE;})==1;
	}

	ToolResponse ToolEditShape::begin(const ToolArgs &args) {
		std::cout << "tool edit shape\n";

		auto padstack = core.a->get_padstack();

		auto uu = std::find_if(core.r->selection.begin(), core.r->selection.end(), [](const auto &x){return x.type == ObjectType::SHAPE;})->uuid;
		auto shape = &padstack->shapes.at(uu);

		bool r = imp->dialogs.edit_shape(shape);
		if(r) {
			core.r->commit();
		}
		else {
			core.r->revert();
		}
		return ToolResponse::end();
	}
	ToolResponse ToolEditShape::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
