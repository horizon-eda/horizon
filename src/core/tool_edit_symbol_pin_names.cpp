#include "tool_edit_symbol_pin_names.hpp"
#include "core_schematic.hpp"
#include "imp_interface.hpp"
#include <iostream>

namespace horizon {

	ToolEditSymbolPinNames::ToolEditSymbolPinNames(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	SchematicSymbol *ToolEditSymbolPinNames::get_symbol() {
		SchematicSymbol *rsym = nullptr;
		for(const auto &it : core.r->selection) {
			if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				auto sym = core.c->get_schematic_symbol(it.uuid);
				if(rsym) {
					if(rsym != sym) {
						return nullptr;
					}
				}
				else {
					rsym = sym;
				}
			}
		}
		return rsym;
	}

	bool ToolEditSymbolPinNames::can_begin() {
		return get_symbol();
	}

	ToolResponse ToolEditSymbolPinNames::begin(const ToolArgs &args) {
		SchematicSymbol *sym= get_symbol();
		if(!sym) {
			return ToolResponse::end();
		}
		bool r = imp->dialogs.edit_symbol_pin_names(sym);
		if(r) {
			core.r->commit();
		}
		else {
			core.r->revert();
		}
		return ToolResponse::end();
	}
	ToolResponse ToolEditSymbolPinNames::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
