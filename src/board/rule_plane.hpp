#pragma once
#include "rules/rule.hpp"
#include "common/common.hpp"
#include "rules/rule_match.hpp"
#include "board/plane.hpp"

namespace horizon {
	class RulePlane: public Rule {
		public:
			RulePlane(const UUID &uu);
			RulePlane(const UUID &uu, const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

			RuleMatch match;
			int layer = 10000;

			PlaneSettings settings;
	};
}
