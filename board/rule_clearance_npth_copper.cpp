#include "rule_clearance_npth_copper.hpp"
#include "util.hpp"
#include "lut.hpp"
#include <sstream>

namespace horizon {


	RuleClearanceNPTHCopper::RuleClearanceNPTHCopper(const UUID &uu): Rule(uu) {
		id = RuleID::CLEARANCE_NPTH_COPPER;
	}

	RuleClearanceNPTHCopper::RuleClearanceNPTHCopper(const UUID &uu, const json &j): Rule(uu, j),
			match(j.at("match")), layer(j.at("layer")), clearance(j.value("clearance", 0.1_mm)){
		id = RuleID::CLEARANCE_NPTH_COPPER;
		{
		}
	}

	json RuleClearanceNPTHCopper::serialize() const {
		json j = Rule::serialize();
		j["match"] = match.serialize();
		j["layer"] = layer;
		j["clearance"] = clearance;
		return j;
	}


	std::string RuleClearanceNPTHCopper::get_brief(const class Block *block) const {
		std::stringstream ss;
		ss<<"Match " << match.get_brief(block) << "\n";
		ss<<"Layer " << layer;
		return ss.str();
	}

}
