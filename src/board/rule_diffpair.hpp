#pragma once
#include "rules/rule.hpp"
#include "common/common.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
	class RuleDiffpair: public Rule {
		public:
			RuleDiffpair(const UUID &uu);
			RuleDiffpair(const UUID &uu, const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

			UUID net_class;
			int layer = 10000;

			uint64_t track_width = .2_mm;
			uint64_t track_gap = .2_mm;
			uint64_t via_gap = .2_mm;
	};
}
