#include "rule_parameters.hpp"
#include "util.hpp"
#include <sstream>

namespace horizon {
	RuleParameters::RuleParameters(): Rule() {
		id = RuleID::PARAMETERS;
	}

	RuleParameters::RuleParameters(const json &j): Rule(j) {
		id = RuleID::PARAMETERS;
		solder_mask_expansion = j.at("solder_mask_expansion");
		paste_mask_contraction = j.at("paste_mask_contraction");
		courtyard_expansion = j.at("courtyard_expansion");
	}

	json RuleParameters::serialize() const {
		json j = Rule::serialize();
		j["solder_mask_expansion"] = solder_mask_expansion;
		j["paste_mask_contraction"] = paste_mask_contraction;
		j["courtyard_expansion"] = courtyard_expansion;
		return j;
	}

	std::string RuleParameters::get_brief(const class Block *block) const {
		return "";
	}

}
