#include "rules_with_core.hpp"
#include "core/core_board.hpp"
#include "core/core_schematic.hpp"
#include "core/core_package.hpp"
#include "board/board_rules.hpp"
#include "schematic/schematic_rules.hpp"

namespace horizon {
	RulesCheckResult rules_check(Rules *r, RuleID id, class Core *c, RulesCheckCache &cache) {
		if(auto rules = dynamic_cast<BoardRules*>(r)) {
			auto core = dynamic_cast<CoreBoard*>(c);
			return rules->check(id, core->get_board(), cache);
		}
		if(auto rules = dynamic_cast<SchematicRules*>(r)) {
			auto core = dynamic_cast<CoreSchematic*>(c);
			return rules->check(id, core->get_schematic(), cache);
		}
		if(auto rules = dynamic_cast<PackageRules*>(r)) {
			auto core = dynamic_cast<CorePackage*>(c);
			return rules->check(id, core->get_package(), cache);
		}
		return RulesCheckResult();
	}
	void rules_apply(Rules *r, RuleID id, class Core *c) {
		if(auto rules = dynamic_cast<BoardRules*>(r)) {
			auto core = dynamic_cast<CoreBoard*>(c);
			rules->apply(id, core->get_board(false), *core->get_via_padstack_provider());
		}
	}
}
