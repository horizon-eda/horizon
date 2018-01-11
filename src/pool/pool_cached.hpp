#pragma once
#include "pool/pool.hpp"

namespace horizon {
	class PoolCached : public Pool{
		public:
			PoolCached(const std::string &base_path, const std::string &cache_path);
			const std::string &get_cache_path() const;
			std::string get_filename(ObjectType type, const UUID &uu) override;

		private:
			std::string cache_path;

	};
}
