#pragma once
#include "rules/rule.hpp"
#include "common/common.hpp"

namespace horizon {
	class RuleHoleSize: public Rule {
		public:
			RuleHoleSize(const UUID &uu);
			RuleHoleSize(const UUID &uu, const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;


			RuleMatch match;
			uint64_t diameter_min = 0.1_mm;
			uint64_t diameter_max = 10_mm;



	};
}
