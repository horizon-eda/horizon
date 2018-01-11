#include "rule_plane.hpp"
#include "util/util.hpp"
#include <sstream>

namespace horizon {
	RulePlane::RulePlane(const UUID &uu): Rule(uu) {
		id = RuleID::PLANE;
	}

	RulePlane::RulePlane(const UUID &uu, const json &j): Rule(uu, j),
		match(j.at("match")),
		layer(j.at("layer")),
		settings(j.at("settings")){
		id = RuleID::PLANE;
	}

	json RulePlane::serialize() const {
		json j = Rule::serialize();
		j["match"] = match.serialize();
		j["layer"] = layer;
		j["settings"] = settings.serialize();
		return j;
	}

	std::string RulePlane::get_brief(const class Block *block) const {
		return "Match " + match.get_brief(block) + "\nLayer " + std::to_string(layer);
	}

}
