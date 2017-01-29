#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "object.hpp"
#include <yaml-cpp/yaml.h>
#include <vector>
#include <map>
#include <set>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;
	

	class Pin {
		public :
			enum class Direction {INPUT, OUTPUT, POWER_INPUT, PASSIVE};

			Pin(const UUID &uu, const json &j);
			Pin(const UUID &uu, const YAML::Node &n);
			Pin(const UUID &uu);

			const UUID uuid;
			std::string primary_name;
			Direction direction = Direction::INPUT;
			unsigned int swap_group = 0;
			std::vector<std::string> names;

			json serialize() const;
			void serialize_yaml(YAML::Emitter &em) const;
	};
	
	class Unit {
		private :
			Unit(const UUID &uu, const json &j);
		
		public :
			static Unit new_from_file(const std::string &filename);
			Unit(const UUID &uu);
			Unit(const UUID &uu, const YAML::Node &n);
			UUID uuid;
			std::string name;
			std::map<UUID, Pin> pins;
			json serialize() const;
			void serialize_yaml(YAML::Emitter &em) const;
	};
	
}
