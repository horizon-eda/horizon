#include "hole.hpp"

namespace horizon {
	using json = nlohmann::json;

	Hole::Hole(const UUID &uu, const json &j):
		uuid(uu),
		position(j.at("position").get<std::vector<int64_t>>()),
		diameter(j.at("diameter").get<uint64_t>()),
		plated(j.at("plated").get<bool>())
	{
	}

	UUID Hole::get_uuid() const {
		return uuid;
	}

	Hole::Hole(const UUID &uu): uuid(uu) {}

	json Hole::serialize() const {
		json j;
		j["position"] = position.as_array();
		j["diameter"] = diameter;
		j["plated"] = plated;
		return j;
	}
}
