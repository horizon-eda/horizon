#pragma once
#include "rules/rule.hpp"
#include "common/common.hpp"
#include "rules/rule_match.hpp"
#include "parameter/set.hpp"

namespace horizon {
	class RuleVia: public Rule {
		public:
			RuleVia(const UUID &uu);
			RuleVia(const UUID &uu, const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

			RuleMatch match;
			UUID padstack;
			ParameterSet parameter_set;
	};
}
