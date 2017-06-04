#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "unit.hpp"
#include "uuid_provider.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Gate : public UUIDProvider {
		public :
			Gate(const UUID &uu, const json &, class Pool &pool);
			Gate(const UUID &uu);
			Gate(const UUID &uu, const YAML::Node &n, Pool &pool);
			virtual UUID get_uuid()const ;
			UUID uuid;
			std::string name;
			std::string suffix;
			unsigned int swap_group = 0;
			Unit *unit = nullptr;

			json serialize() const;
			void serialize_yaml(YAML::Emitter &em) const;
	};

}
