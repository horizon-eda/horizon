#include "tool_smash.hpp"
#include <iostream>
#include "core_schematic.hpp"
#include "core_board.hpp"

namespace horizon {

	ToolSmash::ToolSmash(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	bool ToolSmash::can_begin() {
		for(const auto &it : core.r->selection) {
			if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
				auto sym = core.c->get_schematic_symbol(it.uuid);
				if(sym->smashed == (tool_id==ToolID::UNSMASH))
					return true;
			}
			else if(it.type == ObjectType::BOARD_PACKAGE) {
				auto pkg = &core.b->get_board()->packages.at(it.uuid);
				if(pkg->smashed == (tool_id==ToolID::UNSMASH))
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
			if(it.type == ObjectType::BOARD_PACKAGE) {
				if(tool_id == ToolID::SMASH)
					core.b->get_board()->smash_package(&core.b->get_board()->packages.at(it.uuid));
				else
					core.b->get_board()->unsmash_package(&core.b->get_board()->packages.at(it.uuid));
			}
		}
		core.r->commit();
		return ToolResponse::end();
	}
	ToolResponse ToolSmash::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
