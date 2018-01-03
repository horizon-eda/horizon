#include "tool_set_via_net.hpp"
#include "core_board.hpp"
#include "imp_interface.hpp"
#include <iostream>

namespace horizon {

	ToolSetViaNet::ToolSetViaNet(Core *c, ToolID tid): ToolBase(c, tid) {
	}

	std::set<Via*> ToolSetViaNet::get_vias() {
		std::set<Via*> vias;
		for(const auto &it: core.r->selection) {
			if(it.type == ObjectType::VIA) {
				auto via = &core.b->get_board()->vias.at(it.uuid);
				if(tool_id == ToolID::SET_VIA_NET) {
					if(via->net_set == via->junction->net) {
						vias.insert(via);
					}
				}
				else {
					if(via->net_set) {
						vias.insert(via);
					}
				}
			}
		}
		return vias;
	}

	bool ToolSetViaNet::can_begin() {
		if(!core.b)
			return false;
		return get_vias().size() > 0;
	}

	ToolResponse ToolSetViaNet::begin(const ToolArgs &args) {
		if(!can_begin())
			return ToolResponse::end();

		auto vias = get_vias();

		if(tool_id == ToolID::CLEAR_VIA_NET) {
			for(auto via: vias)
				via->net_set = nullptr;
			core.r->commit();
			return ToolResponse::end();
		}

		if(tool_id == ToolID::SET_VIA_NET) {
			auto r = imp->dialogs.select_net(core.b->get_block(), false);
			if(r.first) {
				auto net = &core.b->get_block()->nets.at(r.second);
				for(auto via: vias)
					via->net_set = net;
			}
		}
		core.r->commit();
		return ToolResponse::end();
	}
	ToolResponse ToolSetViaNet::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
