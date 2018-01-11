#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "rules/rules.hpp"
#include "rule_single_pin_net.hpp"

namespace horizon {
	using json = nlohmann::json;

	class SchematicRules: public Rules {
		public:
			SchematicRules();

			void load_from_json(const json &j);
			RulesCheckResult check(RuleID id, const class Schematic *sch, class RulesCheckCache &cache);
			void apply(RuleID id, class Schematic *sch);
			json serialize() const;
			std::set<RuleID> get_rule_ids() const;
			Rule *get_rule(RuleID id);
			Rule *get_rule(RuleID id, const UUID &uu);
			std::map<UUID, Rule*> get_rules(RuleID id);
			void remove_rule(RuleID id, const UUID &uu);
			Rule *add_rule(RuleID id);

		private:
			RuleSinglePinNet rule_single_pin_net;

			RulesCheckResult check_single_pin_net(const class Schematic *sch, class RulesCheckCache &cache);
	};
}
