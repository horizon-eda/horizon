#include "rule_single_pin_net.hpp"
#include "util/util.hpp"
#include <sstream>

namespace horizon {
	RuleSinglePinNet::RuleSinglePinNet(): Rule() {
		id = RuleID::SINGLE_PIN_NET;
	}

	RuleSinglePinNet::RuleSinglePinNet(const json &j): Rule(j) {
		id = RuleID::SINGLE_PIN_NET;
		include_unnamed = j.at("include_unnamed");
	}

	json RuleSinglePinNet::serialize() const {
		json j = Rule::serialize();
		j["include_unnamed"] = include_unnamed;
		return j;
	}

	std::string RuleSinglePinNet::get_brief(const class Block *block) const {
		return "";
	}

}
