#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include <stdint.h>

namespace horizon {
	using json = nlohmann::json;

	class NetClass {
		public :
			NetClass(const UUID &uu, const json &j);
			NetClass(const UUID &uu);
			UUID uuid;
			std::string name;

			json serialize() const;
	};

}
