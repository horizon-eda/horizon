#include "tool_map_package.hpp"
#include <iostream>
#include "core_board.hpp"

namespace horizon {

	ToolMapPackage::ToolMapPackage(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Map Symbol";
	}

	bool ToolMapPackage::can_begin() {
		return core.b;
	}

	ToolResponse ToolMapPackage::begin(const ToolArgs &args) {
		std::cout << "tool map pkg\n";
		Board *brd = core.b->get_board();
		//collect gates
		std::set<const Component*> components;
		for(const auto &it: brd->block->components) {
			if(it.second.part)
				components.emplace(&it.second);
		}

		for(const auto &it: brd->packages) {
			components.erase(it.second.component);
		}

		for(auto it: components) {
			std::cout << "can map " << it->refdes << std::endl;
		}


		UUID selected_component;
		bool r;
		std::tie(r, selected_component) = core.r->dialogs.map_package(components);
		if(!r) {
			return ToolResponse::end();
		}
		Component *comp = &brd->block->components.at(selected_component);

		auto uu = UUID::random();
		BoardPackage *pkg = &brd->packages.emplace(std::make_pair(uu, BoardPackage(uu, comp))).first->second;
		pkg->placement.shift = args.coords;

		core.r->selection.clear();
		core.r->selection.emplace(pkg->uuid, ObjectType::BOARD_PACKAGE);
		core.r->commit();

		return ToolResponse::next(ToolID::MOVE);
	}
	ToolResponse ToolMapPackage::update(const ToolArgs &args) {
		return ToolResponse();
	}
}
