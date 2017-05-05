#include "rule_descr.hpp"


namespace horizon {
	const std::map<RuleID, RuleDescription> rule_descriptions = {
		{RuleID::HOLE_SIZE, {"Hole size", true}},
		{RuleID::TRACK_WIDTH, {"Track width", true}},
		{RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER, {"Clearance\nSilkscreen - Exposed copper", false}},
	};
}
