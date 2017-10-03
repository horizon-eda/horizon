#pragma once
#include "rules/rule.hpp"
#include "common.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
	class RuleClearanceNPTHCopper: public Rule {
		public:
			RuleClearanceNPTHCopper(const UUID &uu);
			RuleClearanceNPTHCopper(const UUID &uu, const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

			RuleMatch match;
			int layer = 10000;
			uint64_t clearance = 0.1_mm;

		private:

	};
}
