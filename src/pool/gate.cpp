#include "pool/gate.hpp"
#include "pool/pool.hpp"

namespace horizon {

	Gate::Gate(const UUID &uu, const json &j, Pool &pool):
			uuid(uu),
			name(j.at("name").get<std::string>()),
			suffix(j.at("suffix").get<std::string>()),
			swap_group(j.value("swap_group", 0)),
			unit(pool.get_unit(j.at("unit").get<std::string>()))

		{
		}

	Gate::Gate(const UUID &uu): uuid(uu) {}

	Gate::Gate(const UUID &uu, const YAML::Node &n, Pool &pool) :
		uuid(uu),
		name(n["name"].as<std::string>()),
		suffix(n["suffix"].as<std::string>(name)),
		swap_group(n["swap_group"].as<unsigned int>(0)),
		unit(pool.get_unit(n["unit"].as<std::string>()))
	{}

	UUID Gate::get_uuid() const {
		return uuid;
	}

	json Gate::serialize() const {
		json j;
		j["name"] = name;
		j["suffix"] = suffix;
		j["swap_group"] = swap_group;
		j["unit"] = (std::string)unit->uuid;
		return j;
	}

	void Gate::serialize_yaml(YAML::Emitter &em) const {
		using namespace YAML;
		em << BeginMap;
		em << Key << "name" << Value << name;
		em << Key << "suffix" << Value << suffix;
		em << Key << "uuid" << Value << (std::string)uuid;
		em << Key << "unit" << Value << (std::string)unit->uuid;
		em << Key << "unit_name" << Value << unit->name;
		em << Key << "swap_group" << Value << swap_group;
		em << EndMap;
	}
}
