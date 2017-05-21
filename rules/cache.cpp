#include "cache.hpp"
#include "core/core_board.hpp"

namespace horizon {
	RulesCheckCache::RulesCheckCache(Core *c): core(c) {}

	void RulesCheckCache::clear() {
		cache.clear();
	}

	RulesCheckCacheBoardImage::RulesCheckCacheBoardImage(Core *c) {
		auto core = dynamic_cast<CoreBoard*>(c);
		canvas.set_core(c);
		canvas.update(*core->get_board());
	}

	const CanvasPatch *RulesCheckCacheBoardImage::get_canvas() const {
		return &canvas;
	}

	RulesCheckCacheBase *RulesCheckCache::get_cache(RulesCheckCacheID id) {
		if(!cache.count(id)) {
			switch(id) {
				case RulesCheckCacheID::NONE :
				break;
				case RulesCheckCacheID::BOARD_IMAGE :
					cache.emplace(id, std::make_unique<RulesCheckCacheBoardImage>(core));
				break;
			}
		}
		return cache.at(id).get();
	}
}
