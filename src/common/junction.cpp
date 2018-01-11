#include "common/junction.hpp"
#include "common/lut.hpp"

namespace horizon {
	using json = nlohmann::json;
	
	Junction::Junction(const UUID &uu, const json &j):
		uuid(uu),
		position(j.at("position").get<std::vector<int64_t>>()),
		temp(false)
	{
	}
	
	UUID Junction::get_uuid() const {
		return uuid;
	}

	Junction::Junction(const UUID &uu): uuid(uu) {}

	json Junction::serialize() const {
		json j;
			j["position"] = position.as_array();
		return j;
	}
}
