#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "util/uuid_provider.hpp"
#include <stdint.h>

namespace horizon {
	using json = nlohmann::json;

	class NetClass: public UUIDProvider {
		public :
			NetClass(const UUID &uu, const json &j);
			NetClass(const UUID &uu);
			UUID uuid;
			std::string name;
			bool is_default = false;
			UUID get_uuid() const;

			json serialize() const;
	};

}
