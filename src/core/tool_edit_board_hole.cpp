#include "tool_edit_board_hole.hpp"
#include "core_board.hpp"
#include <iostream>
#include "imp/imp_interface.hpp"

namespace horizon {

	ToolEditBoardHole::ToolEditBoardHole(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	bool ToolEditBoardHole::can_begin() {
		return get_holes().size()>0;
	}

	std::set<BoardHole*> ToolEditBoardHole::get_holes() {
		std::set<BoardHole*> holes;
		for(const auto &it : core.r->selection) {
			if(it.type == ObjectType::BOARD_HOLE) {
				holes.emplace(&core.b->get_board()->holes.at(it.uuid));
			}
		}
		return holes;
	}

	ToolResponse ToolEditBoardHole::begin(const ToolArgs &args) {
		auto holes = get_holes();
		auto r = imp->dialogs.edit_board_hole(holes, core.r->m_pool, core.b->get_block());
		if(r) {
			core.r->commit();
		}
		else {
			core.r->revert();
		}
		return ToolResponse::end();
	}
	ToolResponse ToolEditBoardHole::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
