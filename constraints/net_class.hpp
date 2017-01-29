#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include <stdint.h>

namespace horizon {
	using json = nlohmann::json;

	class NetClass {
		public :
			NetClass(const UUID &uu, const json &j);
			NetClass(const UUID &uu);
			UUID uuid;
			std::string name;
			uint64_t min_width;
			uint64_t default_width;

			json serialize() const;
	};

}
