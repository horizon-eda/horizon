#include "rules_with_core.hpp"
#include "core/core_board.hpp"
#include "board/board_rules.hpp"

namespace horizon {
	RulesCheckResult rules_check(Rules *r, RuleID id, class Core *c) {
		if(auto rules = dynamic_cast<BoardRules*>(r)) {
			auto core = dynamic_cast<CoreBoard*>(c);
			return rules->check(id, core->get_board());
		}
		return RulesCheckResult();
	}
	void rules_apply(Rules *r, RuleID id, class Core *c) {
		if(auto rules = dynamic_cast<BoardRules*>(r)) {
			auto core = dynamic_cast<CoreBoard*>(c);
			rules->apply(id, core->get_board(false));
		}
	}
}
