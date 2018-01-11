#pragma once
#include "rules/rule.hpp"
#include "common/common.hpp"

namespace horizon {
	class RulePackageChecks: public Rule {
		public:
			RulePackageChecks();
			RulePackageChecks(const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

	};
}
