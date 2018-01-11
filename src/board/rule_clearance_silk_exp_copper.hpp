#pragma once
#include "rules/rule.hpp"
#include "common/common.hpp"

namespace horizon {
	class RuleClearanceSilkscreenExposedCopper: public Rule {
		public:
			RuleClearanceSilkscreenExposedCopper();
			RuleClearanceSilkscreenExposedCopper(const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

			uint64_t clearance_top = 0.1_mm;
			uint64_t clearance_bottom = 0.1_mm;
	};
}
