#include "rules/rule.hpp"
#include "common/common.hpp"

namespace horizon {
	class RuleSinglePinNet: public Rule {
		public:
			RuleSinglePinNet();
			RuleSinglePinNet(const json &j);
			json serialize() const override;

			std::string get_brief(const class Block *block = nullptr) const;

			bool include_unnamed = true;
	};
}
