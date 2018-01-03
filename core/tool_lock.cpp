#include "tool_lock.hpp"
#include "core_board.hpp"
#include <iostream>

namespace horizon {

	ToolLock::ToolLock(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	std::set<Via*> ToolLock::get_vias(bool locked) {
		std::set<Via*> vias;
		for(const auto &it: core.r->selection) {
			if(it.type == ObjectType::VIA) {
				auto via = &core.b->get_board()->vias.at(it.uuid);
				if(via->locked == locked)
					vias.insert(via);
			}
		}
		return vias;
	}
	std::set<Track*> ToolLock::get_tracks(bool locked) {
		std::set<Track*> tracks;
		for(const auto &it: core.r->selection) {
			if(it.type == ObjectType::TRACK) {
				auto track = &core.b->get_board()->tracks.at(it.uuid);
				if(track->locked == locked)
					tracks.insert(track);
			}
		}
		return tracks;
	}

	bool ToolLock::can_begin() {
		if(!core.b)
			return false;
		if(tool_id == ToolID::UNLOCK_ALL) {
			return true;
		}
		else if(tool_id == ToolID::LOCK) {
			return (get_vias(false).size() + get_tracks(false).size()) > 0;
		}
		else if(tool_id == ToolID::UNLOCK) {
			return (get_vias(true).size() + get_tracks(true).size()) > 0;
		}
		return false;
	}

	bool ToolLock::is_specific() {
		return tool_id != ToolID::UNLOCK_ALL;
	}

	ToolResponse ToolLock::begin(const ToolArgs &args) {
		if(tool_id == ToolID::UNLOCK_ALL) {
			auto brd = core.b->get_board();
			for(auto &it: brd->vias) {
				it.second.locked = false;
			}
			for(auto &it: brd->tracks) {
				it.second.locked = false;
			}
			core.r->commit();
			return ToolResponse::end();
		}
		else {
			for(auto via: get_vias(tool_id != ToolID::LOCK))
				via->locked = tool_id == ToolID::LOCK;
			for(auto track: get_tracks(tool_id != ToolID::LOCK))
				track->locked = tool_id == ToolID::LOCK;
			core.r->commit();
			return ToolResponse::end();
		}

		return ToolResponse::end();
	}
	ToolResponse ToolLock::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
