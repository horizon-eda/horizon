#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "rules/rules.hpp"
#include "rule_package_checks.hpp"

namespace horizon {
	using json = nlohmann::json;

	class PackageRules: public Rules {
		public:
			PackageRules();

			void load_from_json(const json &j);
			RulesCheckResult check(RuleID id, const class Package *pkg, class RulesCheckCache &cache);
			json serialize() const;
			std::set<RuleID> get_rule_ids() const;
			Rule *get_rule(RuleID id);
			Rule *get_rule(RuleID id, const UUID &uu);
			std::map<UUID, Rule*> get_rules(RuleID id);
			void remove_rule(RuleID id, const UUID &uu);
			Rule *add_rule(RuleID id);

		private:
			RulePackageChecks rule_package_checks;

			RulesCheckResult check_package(const class Package *pkg);
	};
}
