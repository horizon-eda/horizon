#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "pool/unit.hpp"
#include "pool/gate.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Entity: public UUIDProvider {
		private :
			Entity(const UUID &uu, const json &, class Pool &pool);

		public :
			Entity(const UUID &uu);
			Entity(const UUID &uu, const YAML::Node &n, Pool &pool);

			static Entity new_from_file(const std::string &filename, Pool &pool);
			UUID uuid;
			std::string name;
			std::string manufacturer;
			std::string prefix;
			std::set<std::string> tags;
			std::map<UUID, Gate> gates;
			void serialize_yaml(YAML::Emitter &em) const;
			json serialize() const;
			void update_refs(Pool &pool);
			UUID get_uuid() const;
	};

}
