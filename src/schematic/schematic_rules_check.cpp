#include "schematic.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"

namespace horizon {
	RulesCheckResult SchematicRules::check_single_pin_net(const class Schematic *sch, class RulesCheckCache &cache) {
		RulesCheckResult r;
		r.level = RulesCheckErrorLevel::PASS;
		auto rule = dynamic_cast<RuleSinglePinNet*>(get_rule(RuleID::SINGLE_PIN_NET));
		auto c = dynamic_cast<RulesCheckCacheNetPins*>(cache.get_cache(RulesCheckCacheID::NET_PINS));

		for(const auto &it: c->get_net_pins()) {
			if(rule->include_unnamed || it.first->is_named()) {
				if(it.second.size()==1) {
					r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
					auto &x = r.errors.back();
					auto &conn = it.second.at(0);
					x.comment = "Net \"" + it.first->name + "\" only connected to " + std::get<0>(conn)->refdes + std::get<1>(conn)->suffix + "." + std::get<2>(conn)->primary_name;
					x.sheet = std::get<3>(conn);
					x.location = std::get<4>(conn);
					x.has_location = true;
				}
			}
		}
		r.update();
		return r;
	}

	RulesCheckResult SchematicRules::check(RuleID id, const Schematic *sch, RulesCheckCache &cache) {
		switch(id) {
			case RuleID::SINGLE_PIN_NET :
				return check_single_pin_net(sch, cache);

			default:
				return RulesCheckResult();
		}
	}
}
