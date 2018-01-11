#include "schematic_rules.hpp"
#include "schematic.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"

namespace horizon {
	SchematicRules::SchematicRules() {}

	void SchematicRules::load_from_json(const json &j) {
		if(j.count("single_pin_net")) {
			const json &o = j["single_pin_net"];
			rule_single_pin_net = RuleSinglePinNet(o);
		}
	}

	void SchematicRules::apply(RuleID id, Schematic *sch) {
	}

	json SchematicRules::serialize() const {
		json j;
		j["single_pin_net"] = rule_single_pin_net.serialize();

		return j;
	}

	std::set<RuleID> SchematicRules::get_rule_ids() const {
		return {RuleID::SINGLE_PIN_NET};
	}

	Rule *SchematicRules::get_rule(RuleID id) {
		if(id == RuleID::SINGLE_PIN_NET) {
			return &rule_single_pin_net;
		}
		return nullptr;
	}

	Rule *SchematicRules::get_rule(RuleID id, const UUID &uu) {
		return nullptr;
	}

	std::map<UUID, Rule*> SchematicRules::get_rules(RuleID id) {
		std::map<UUID, Rule*> r;
		switch(id) {
			default:;
		}
		return r;
	}

	void SchematicRules::remove_rule(RuleID id, const UUID &uu) {
		switch(id) {
			default:;
		}
		fix_order(id);
	}

	Rule *SchematicRules::add_rule(RuleID id) {
		Rule *r = nullptr;
		switch(id) {
			default:
				return nullptr;
		}
		return r;
	}
}
