#include "rule_clearance_silk_exp_copper.hpp"
#include "util/util.hpp"
#include <sstream>

namespace horizon {
	RuleClearanceSilkscreenExposedCopper::RuleClearanceSilkscreenExposedCopper(): Rule() {
		id = RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER;
	}

	RuleClearanceSilkscreenExposedCopper::RuleClearanceSilkscreenExposedCopper(const json &j): Rule(j) {
		id = RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER;
		clearance_top = j.at("clearance_top");
		clearance_bottom = j.at("clearance_bottom");
	}

	json RuleClearanceSilkscreenExposedCopper::serialize() const {
		json j = Rule::serialize();
		j["clearance_top"] = clearance_top;
		j["clearance_bottom"] = clearance_bottom;
		return j;
	}

	std::string RuleClearanceSilkscreenExposedCopper::get_brief(const class Block *block) const {
		return "";
	}

}
