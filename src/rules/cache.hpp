#pragma once
#include <map>
#include <memory>
#include <deque>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "canvas/canvas_patch.hpp"
#include "pool/entity.hpp"

namespace horizon {
	enum class RulesCheckCacheID {NONE, BOARD_IMAGE, NET_PINS};

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

	class RulesCheckCacheNetPins: public RulesCheckCacheBase {
		public:
			RulesCheckCacheNetPins(class Core *c);
			const std::map<class Net*, std::deque<std::tuple<class Component*, const class Gate *, const class Pin*, UUID, Coordi>>> &get_net_pins() const;

		private :
			std::map<class Net*, std::deque<std::tuple<class Component*, const class Gate *, const class Pin*, UUID, Coordi>>> net_pins;

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
