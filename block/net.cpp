#include "net.hpp"

namespace horizon {

	Net::Net(const UUID &uu, const json &j, const NetClasses &constr):
			uuid(uu),
			name(j.at("name").get<std::string>()),
			is_power(j.value("is_power", false))
		{
			if(j.count("net_class")) {
				net_class = &constr.net_classes.at(j.at("net_class").get<std::string>());
			}
			else {
				net_class = constr.default_net_class;
			}
		}

	Net::Net (const UUID &uu): uuid(uu){};

	UUID Net::get_uuid() const {
		return uuid;
	}

	json Net::serialize() const {
		json j;
		j["name"] = name;
		j["is_power"] = is_power;
		j["net_class"] = net_class->uuid;
		return j;
	}

	bool Net::is_named() const {
		return name.size()>0;
	}
}
