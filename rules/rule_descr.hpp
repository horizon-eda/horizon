#pragma once
#include <string>
#include "rule.hpp"

namespace horizon {
	class RuleDescription {
		public :
		RuleDescription(const std::string &n, bool m):
			name(n),
			multi(m)
		{}

		std::string name;
		bool multi;
	};

	extern const std::map<RuleID, RuleDescription> rule_descriptions;
}
