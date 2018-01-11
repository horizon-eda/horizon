#pragma once
#include "rules/rule.hpp"
#include "common/common.hpp"

namespace horizon {
	class RuleParameters: public Rule {
		public:
			RuleParameters();
			RuleParameters(const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

			uint64_t solder_mask_expansion = 0.1_mm;
			uint64_t paste_mask_contraction = 0;
			uint64_t courtyard_expansion = 0.25_mm;
			uint64_t via_solder_mask_expansion = 0.1_mm;
			uint64_t hole_solder_mask_expansion = 0.1_mm;
	};
}
