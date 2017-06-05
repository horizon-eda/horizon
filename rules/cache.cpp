#include "cache.hpp"
#include "core/core_board.hpp"
#include "core/core_schematic.hpp"

namespace horizon {
	RulesCheckCache::RulesCheckCache(Core *c): core(c) {}

	void RulesCheckCache::clear() {
		cache.clear();
	}

	RulesCheckCacheBoardImage::RulesCheckCacheBoardImage(Core *c) {
		auto core = dynamic_cast<CoreBoard*>(c);
		canvas.update(*core->get_board());
	}

	const CanvasPatch *RulesCheckCacheBoardImage::get_canvas() const {
		return &canvas;
	}

	RulesCheckCacheNetPins::RulesCheckCacheNetPins(Core *c) {
		auto core = dynamic_cast<CoreSchematic *>(c);
		assert(core);
		auto block = core->get_schematic()->block;
		for(auto &it: block->nets) {
			net_pins[&it.second];
		}
		for(auto &it: block->components) {
			for(auto &it_conn: it.second.connections) {
				const auto &connpath = it_conn.first;
				auto gate = &it.second.entity->gates.at(connpath.at(0));
				auto pin = &gate->unit->pins.at(connpath.at(1));
				UUID sheet_uuid;
				Coordi location;
				for(const auto &it_sheet: core->get_schematic()->sheets) {
					bool found = false;
					for(const auto &it_sym: it_sheet.second.symbols) {
						if(it_sym.second.component == &it.second && it_sym.second.gate == gate) {
							found = true;
							sheet_uuid = it_sheet.first;
							location = it_sym.second.placement.transform(it_sym.second.pool_symbol->pins.at(pin->uuid).position);
							break;
						}
					}
					if(found)
						break;
				}
				net_pins.at(it_conn.second.net.ptr).emplace_back(&it.second, gate, pin, sheet_uuid, location);
			}
		}
	}

	const std::map<class Net*, std::deque<std::tuple<class Component*, const class Gate *, const class Pin*, UUID, Coordi>>> &RulesCheckCacheNetPins::get_net_pins() const {
		return net_pins;
	}

	RulesCheckCacheBase *RulesCheckCache::get_cache(RulesCheckCacheID id) {
		if(!cache.count(id)) {
			switch(id) {
				case RulesCheckCacheID::NONE :
				break;
				case RulesCheckCacheID::BOARD_IMAGE :
					cache.emplace(id, std::make_unique<RulesCheckCacheBoardImage>(core));
				break;
				case RulesCheckCacheID::NET_PINS :
					cache.emplace(id, std::make_unique<RulesCheckCacheNetPins>(core));
				break;
			}
		}
		return cache.at(id).get();
	}
}
