#pragma once
#include <map>
#include <memory>
#include <deque>
#include "uuid.hpp"
#include "common.hpp"
#include "canvas/canvas_patch.hpp"

namespace horizon {
	enum class RulesCheckCacheID {NONE, BOARD_IMAGE};

	class RulesCheckCacheBase {
		public:
			virtual ~RulesCheckCacheBase(){}
	};

	class RulesCheckCacheBoardImage: public RulesCheckCacheBase {
		public:
			RulesCheckCacheBoardImage(class Core *c);
			const CanvasPatch *get_canvas() const;

		private:
			CanvasPatch canvas;
	};

	class RulesCheckCache {
		public:
			RulesCheckCache(class Core *c);
			RulesCheckCacheBase *get_cache(RulesCheckCacheID id);
			void clear();

		private:
			std::map<RulesCheckCacheID, std::unique_ptr<RulesCheckCacheBase>> cache;
			class Core *core;

	};
}
