#include "rules.hpp"

namespace horizon {

	void RulesCheckResult::clear() {
		errors.clear();
		level = RulesCheckErrorLevel::NOT_RUN;
	}

	void RulesCheckResult::update() {
		for(const auto &it: errors) {
			if(static_cast<int>(level) < static_cast<int>(it.level)) {
				level = it.level;
			}
		}
	}

	RulesCheckError::RulesCheckError(RulesCheckErrorLevel lev): level(lev) {}

	Color rules_check_error_level_to_color(RulesCheckErrorLevel lev) {
		switch(lev) {
			case RulesCheckErrorLevel::NOT_RUN:	return Color::new_from_int(136, 138, 133);
			case RulesCheckErrorLevel::PASS:		return Color::new_from_int(138, 226, 52);
			case RulesCheckErrorLevel::WARN:		return Color::new_from_int(252, 175, 62);
			case RulesCheckErrorLevel::FAIL:		return Color::new_from_int(239, 41, 41);
			default: return Color::new_from_int(255, 0, 255);
		}
	}
	std::string rules_check_error_level_to_string(RulesCheckErrorLevel lev) {
		switch(lev) {
			case RulesCheckErrorLevel::NOT_RUN:	return "Not run";
			case RulesCheckErrorLevel::PASS:		return "Pass";
			case RulesCheckErrorLevel::WARN:		return "Warn";
			case RulesCheckErrorLevel::FAIL:		return "Fail";
			default: return "invalid";
		}
	}


	Rules::Rules() {}
	Rules::~Rules() {}
	void Rules::fix_order(RuleID id) {
		auto rv = get_rules_sorted(id);
		int i = 0;
		for(auto it: rv) {
			it->order = i++;
		}
	}

	std::vector<Rule*> Rules::get_rules_sorted(RuleID id) {
		auto rs = get_rules(id);
		std::vector<Rule*> rv;
		rv.reserve(rs.size());
		for(auto &it: rs) {
			rv.push_back(it.second);
		}
		std::sort(rv.begin(), rv.end(), [](auto a, auto b){return a->order < b->order;});
		return rv;
	}

	void Rules::move_rule(RuleID id, const UUID &uu, int dir) {
		auto rules = get_rules(id);
		auto rule = get_rule(id, uu);
		if(dir < 0) {
			dir = -1;
		}
		else {
			dir = 1;
		}
		if(dir < 0 && rule->order==0)
			return;
		if(dir > 0 && rule->order==rules.size()-1)
			return;
		auto rule_other = std::find_if(rules.begin(), rules.end(), [rule, dir](const auto x){return x.second->order == rule->order+dir;});
		assert(rule_other != rules.end());
		std::swap(rule_other->second->order, rule->order);
	}
}
