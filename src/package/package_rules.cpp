#include "package_rules.hpp"
#include "pool/package.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"

namespace horizon {
	PackageRules::PackageRules() {}

	void PackageRules::load_from_json(const json &j) {
		if(j.count("package_checks")) {
			const json &o = j["package_checks"];
			rule_package_checks = RulePackageChecks(o);
		}
	}

	json PackageRules::serialize() const {
		json j;
		j["package_checks"] = rule_package_checks.serialize();

		return j;
	}

	std::set<RuleID> PackageRules::get_rule_ids() const {
		return {RuleID::PACKAGE_CHECKS};
	}

	Rule *PackageRules::get_rule(RuleID id) {
		if(id == RuleID::PACKAGE_CHECKS) {
			return &rule_package_checks;
		}
		return nullptr;
	}

	Rule *PackageRules::get_rule(RuleID id, const UUID &uu) {
		return nullptr;
	}

	std::map<UUID, Rule*> PackageRules::get_rules(RuleID id) {
		std::map<UUID, Rule*> r;
		switch(id) {
			default:;
		}
		return r;
	}

	void PackageRules::remove_rule(RuleID id, const UUID &uu) {
		switch(id) {
			default:;
		}
		fix_order(id);
	}

	Rule *PackageRules::add_rule(RuleID id) {
		Rule *r = nullptr;
		switch(id) {
			default:
				return nullptr;
		}
		return r;
	}
}
