#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "unit.hpp"
#include "util/uuid_provider.hpp"
#include "util/uuid_ptr.hpp"
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
			uuid_ptr<const Unit> unit;

			json serialize() const;
			void serialize_yaml(YAML::Emitter &em) const;
	};

}
