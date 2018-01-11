#include "tool_select_more.hpp"
#include <iostream>
#include "core_board.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

	ToolSelectMore::ToolSelectMore(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	bool ToolSelectMore::can_begin() {
		if(!core.b)
			return false;
		for(const auto &it: core.r->selection) {
			if(it.type == ObjectType::TRACK)
				return true;
			else if(it.type == ObjectType::JUNCTION)
				return true;
			else if(it.type == ObjectType::VIA)
				return true;
		}
		return false;
	}

	ToolResponse ToolSelectMore::begin(const ToolArgs &args) {
		std::cout << "tool select net seg\n";
		std::map<const Junction*, std::set<const Track*>> junction_connections;
		auto brd = core.b->get_board();
		for(const auto &it: brd->tracks) {
			for(const auto &it_ft: {it.second.from, it.second.to}) {
				if(it_ft.is_junc()) {
					junction_connections[it_ft.junc].insert(&it.second);
				}
			}
		}
		std::set<const Junction*> junctions;
		std::set<const Track*> tracks;

		for(const auto &it: args.selection) {
			if(it.type == ObjectType::TRACK) {
				tracks.insert(&brd->tracks.at(it.uuid));
			}
			else if(it.type == ObjectType::JUNCTION) {
				junctions.insert(&brd->junctions.at(it.uuid));
			}
			else if(it.type == ObjectType::VIA) {
				junctions.insert(brd->vias.at(it.uuid).junction);
			}
		}
		bool inserted = true;
		while(inserted) {
			inserted = false;
			for(const auto it: tracks) {
				for(const auto &it_ft: {it->from, it->to}) {
					if(it_ft.is_junc()) {
						if(junctions.insert(it_ft.junc).second)
							inserted = true;
					}
				}
			}
			for(const auto it: junctions) {
				if(junction_connections.count(it)) {
					for(const auto &it_track: junction_connections.at(it)) {
						if(tracks.insert(it_track).second)
							inserted = true;
					}
				}
			}
		}
		core.r->selection.clear();
		for(const auto it: junctions) {
			core.r->selection.emplace(it->uuid, ObjectType::JUNCTION);
		}
		for(const auto it: tracks) {
			core.r->selection.emplace(it->uuid, ObjectType::TRACK);
		}
		return ToolResponse::end();

	}
	ToolResponse ToolSelectMore::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
