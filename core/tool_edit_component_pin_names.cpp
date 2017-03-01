#include "tool_edit_component_pin_names.hpp"
#include "core_schematic.hpp"
#include "imp_interface.hpp"
#include <iostream>

namespace horizon {

	ToolEditComponentPinNames::ToolEditComponentPinNames(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Edit component pin names";
	}

	Component *ToolEditComponentPinNames::get_component() {
		Component *comp = nullptr;
		for(const auto &it : core.r->selection) {
			if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				auto sym = core.c->get_schematic_symbol(it.uuid);
				if(comp) {
					if(comp != sym->component) {
						return nullptr;
					}
				}
				else {
					comp = sym->component;
				}
			}
		}
		return comp;
	}

	bool ToolEditComponentPinNames::can_begin() {
		return get_component();
	}

	ToolResponse ToolEditComponentPinNames::begin(const ToolArgs &args) {
		std::cout << "tool disconnect\n";
		Component *comp = get_component();
		if(!comp) {
			return ToolResponse::end();
		}
		bool r = imp->dialogs.edit_component_pin_names(comp);
		if(r) {
			core.r->commit();
		}
		else {
			core.r->revert();
		}
		return ToolResponse::end();
	}
	ToolResponse ToolEditComponentPinNames::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
