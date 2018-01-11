#include "package/pad.hpp"

namespace horizon {

	Pad::Pad(const UUID &uu, const json &j, Pool &pool):
			uuid(uu),
			pool_padstack(pool.get_padstack(j.at("padstack").get<std::string>())),
			padstack(*pool_padstack),
			placement(j.at("placement")),
			name(j.at("name").get<std::string>())
		{
			if(j.count("parameter_set")) {
				parameter_set = parameter_set_from_json(j.at("parameter_set"));
			}
		}
	Pad::Pad(const UUID &uu, const Padstack *ps): uuid(uu), pool_padstack(ps), padstack(*ps) {}

	json Pad::serialize() const {
			json j;
			j["padstack"] = (std::string)pool_padstack->uuid;
			j["placement"] = placement.serialize();
			j["name"] = name;
			j["parameter_set"] = parameter_set_serialize(parameter_set);


			return j;
		}

	UUID Pad::get_uuid() const {
		return uuid;
	}
}
