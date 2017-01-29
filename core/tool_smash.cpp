#include "tool_smash.hpp"
#include <iostream>
#include "core_schematic.hpp"

namespace horizon {

	ToolSmash::ToolSmash(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	bool ToolSmash::can_begin() {
		for(const auto &it : core.r->selection) {
			if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				return true;
			}
		}
		return false;
	}

	ToolResponse ToolSmash::begin(const ToolArgs &args) {
		std::cout << "tool smash\n";
		for(const auto &it : args.selection) {
			if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				if(tool_id == ToolID::SMASH)
					core.c->get_schematic()->smash_symbol(core.c->get_sheet(), core.c->get_schematic_symbol(it.uuid));
				else
					core.c->get_schematic()->unsmash_symbol(core.c->get_sheet(), core.c->get_schematic_symbol(it.uuid));
			}
		}
		core.c->commit();
		return ToolResponse::end();
	}
	ToolResponse ToolSmash::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
