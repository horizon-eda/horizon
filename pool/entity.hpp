#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "object.hpp"
#include "unit.hpp"
#include "gate.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Entity {
		private :
			Entity(const UUID &uu, const json &, Object &objj);

		public :
			Entity(const UUID &uu);
			Entity(const UUID &uu, const YAML::Node &n, Object &objj);

			static Entity new_from_file(const std::string &filename, Object &obj);
			UUID uuid;
			std::string name;
			std::string prefix;
			std::set<std::string> tags;
			std::map<UUID, Gate> gates;
			void serialize_yaml(YAML::Emitter &em) const;
			json serialize() const;
	};

}
