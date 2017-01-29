#include "net_class.hpp"
#include "json.hpp"
#include "common.hpp"

namespace horizon {
	NetClass::NetClass(const UUID &uu, const json &j):
		uuid(uu),
		name(j.at("name").get<std::string>()),
		min_width(j.at("min_width")),
		default_width(j.at("default_width"))
	{

	}

	NetClass::NetClass(const UUID &uu): uuid(uu), name("default"), min_width(0.2_mm), default_width(0.5_mm) {}

	json NetClass::serialize() const {
		json j;
		j["name"] = name;
		j["min_width"] = min_width;
		j["default_width"] = default_width;
		return j;
	}
}
