#pragma once
#include <map>
#include <memory>
#include <deque>
#include "uuid.hpp"
#include "common.hpp"

namespace horizon {
	enum class CheckCacheID {NONE, NET_PINS};

	class CheckCacheBase {

		public:
			virtual ~CheckCacheBase(){}
	};

	class CheckCacheNetPins: public CheckCacheBase {
		public:
			CheckCacheNetPins(class Core *c);
			std::map<class Net*, std::deque<std::tuple<class Component*, const class Gate *, const class Pin*, UUID, Coordi>>> net_pins;

	};

	class CheckCache {
		public:
			CheckCache(class Core *c);
			CheckCacheBase *get_cache(CheckCacheID id);
			void clear();

		private:
			std::map<CheckCacheID, std::unique_ptr<CheckCacheBase>> cache;
			class Core *core;

	};
}
