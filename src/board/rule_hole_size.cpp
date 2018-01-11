#include "rule_hole_size.hpp"
#include "util/util.hpp"
#include <sstream>

namespace horizon {
	RuleHoleSize::RuleHoleSize(const UUID &uu): Rule(uu) {
		id = RuleID::HOLE_SIZE;
	}

	RuleHoleSize::RuleHoleSize(const UUID &uu, const json &j): Rule(uu, j),
			diameter_min(j.at("diameter_min")),
			diameter_max(j.at("diameter_max")),
			match(j.at("match")){
		id = RuleID::HOLE_SIZE;
	}

	json RuleHoleSize::serialize() const {
		json j = Rule::serialize();
		j["diameter_min"] = diameter_min;
		j["diameter_max"] = diameter_max;
		j["match"] = match.serialize();
		return j;
	}

	std::string RuleHoleSize::get_brief(const class Block *block) const {
		return "Match " + match.get_brief(block);
	}

}
