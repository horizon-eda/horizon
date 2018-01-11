#pragma once
#include "rules/rule.hpp"
#include "common/common.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
	class RuleClearanceCopperOther: public Rule {
		public:
			RuleClearanceCopperOther(const UUID &uu);
			RuleClearanceCopperOther(const UUID &uu, const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

			uint64_t get_clearance(PatchType pt_copper, PatchType pt_non_copper) const;
			void set_clearance(PatchType pt_copper, PatchType pt_non_copper, uint64_t c);
			uint64_t get_max_clearance() const;

			RuleMatch match;
			int layer = 10000;
			uint64_t routing_offset = 0.05_mm;

		private:
			std::map<std::pair<PatchType, PatchType>, uint64_t> clearances;

	};
}
