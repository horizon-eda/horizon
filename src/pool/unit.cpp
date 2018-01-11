#include "unit.hpp"
#include "common/lut.hpp"

namespace horizon {
	static const LutEnumStr<Pin::Direction> pin_direction_lut = {
		{"output", 			Pin::Direction::OUTPUT},
		{"input", 			Pin::Direction::INPUT},
		{"bidirectional", 	Pin::Direction::BIDIRECTIONAL},
		{"open_collector", 	Pin::Direction::OPEN_COLLECTOR},
		{"power_input",		Pin::Direction::POWER_INPUT},
		{"power_output",	Pin::Direction::POWER_OUTPUT},
		{"passive",			Pin::Direction::PASSIVE},
	};

	Pin::Pin(const UUID &uu, const json &j):
		uuid(uu),
		primary_name(j.at("primary_name").get<std::string>()),
		direction(pin_direction_lut.lookup(j.at("direction"))),
		swap_group(j.value("swap_group", 0)),
		names(j.value("names", std::vector<std::string>()))
	{
	}
	
	Pin::Pin(const UUID &uu, const YAML::Node &n) :
		uuid(uu),
		primary_name(n["primary_name"].as<std::string>()),
		direction(pin_direction_lut.lookup(n["direction"].as<std::string>("input"))),
		swap_group(n["swap_group"].as<int>(0)),
		names(n["names"].as<std::vector<std::string>>(std::vector<std::string>()))
	{}
	
	Pin::Pin(const UUID &uu): uuid(uu) {}


	json Pin::serialize() const {
		json j;
		j["primary_name"] = primary_name;
		j["direction"] = pin_direction_lut.lookup_reverse(direction);
		j["swap_group"] = swap_group;
		j["names"] = names;
		return j;
	}

	void Pin::serialize_yaml(YAML::Emitter &em) const {
		using namespace YAML;
		em << BeginMap;
		em << Key << "primary_name" << Value << primary_name;
		em << Key << "uuid" << Value << (std::string)uuid;
		em << Key << "names" << Value << names;
		em << Key << "direction" << Value << pin_direction_lut.lookup_reverse(direction);
		em << Key << "swap_group" << Value << swap_group;
		em << EndMap;
		/*YAML::Node n;
		n["uuid"] = (std::string)uuid;
		n["primary_name"] = primary_name;
		n["names"] = names;
		n["direction"] = pin_direction_lut.lookup_reverse(direction);
		return n;*/
	}

	UUID Pin::get_uuid() const {
		return uuid;
	}

	Unit::Unit(const UUID &uu, const json &j):
		uuid(uu),
		name(j["name"].get<std::string>()),
		manufacturer(j.value("manufacturer", ""))
	{
		const json &o = j["pins"];
		for (auto it = o.cbegin(); it != o.cend(); ++it) {
			auto pin_uuid = UUID(it.key());
			pins.insert(std::make_pair(pin_uuid, Pin(pin_uuid, it.value())));
		}
	}
	
	Unit::Unit(const UUID &uu, const YAML::Node &n) :
		uuid(uu),
		name(n["name"].as<std::string>()),
		manufacturer(n["manufacturer"].as<std::string>())
	{
		for(const auto &it: n["pins"]) {
			UUID pin_uuid = it["uuid"].as<std::string>(UUID::random());
			pins.insert(std::make_pair(pin_uuid, Pin(pin_uuid, it)));
		}
	}

	Unit::Unit(const UUID &uu):uuid(uu) {}

	Unit Unit::new_from_file(const std::string &filename) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return Unit(UUID(j["uuid"].get<std::string>()), j);
	}

	json Unit::serialize() const {
		json j;
		j["type"] = "unit";
		j["name"] = name;
		j["manufacturer"] = manufacturer;
		j["uuid"] = (std::string)uuid;
		j["pins"] = json::object();
		for(const auto &it: pins) {
			j["pins"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}

	void Unit::serialize_yaml(YAML::Emitter &em) const {
		using namespace YAML;
		em << BeginMap;
		em << Key << "name" << Value << name;
		em << Key << "manufacturer" << Value << manufacturer;
		em << Key << "uuid" << Value << (std::string)uuid;
		em << Key << "pins" << Value << BeginSeq;
		for(const auto &it: pins) {
			it.second.serialize_yaml(em);
		}
		em << EndSeq;
		em << EndMap;
	}

	UUID Unit::get_uuid() const {
		return uuid;
	}

}
