#include "net_class.hpp"
#include "common/common.hpp"

namespace horizon {
	NetClass::NetClass(const UUID &uu, const json &j):
		uuid(uu),
		name(j.at("name").get<std::string>())
	{

	}

	NetClass::NetClass(const UUID &uu): uuid(uu), name("default") {}

	json NetClass::serialize() const {
		json j;
		j["name"] = name;
		return j;
	}

	UUID NetClass::get_uuid() const {
		return uuid;
	}
}
