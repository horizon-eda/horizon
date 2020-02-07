#include "rules_with_core.hpp"
#include "board/board_rules.hpp"
#include "document/idocument_board.hpp"
#include "document/idocument_schematic.hpp"
#include "document/idocument_package.hpp"
#include "schematic/schematic_rules.hpp"
#include "package/package_rules.hpp"

namespace horizon {
RulesCheckResult rules_check(Rules *r, RuleID id, class IDocument *c, RulesCheckCache &cache,
                             check_status_cb_t status_cb)
{
    if (auto rules = dynamic_cast<BoardRules *>(r)) {
        auto core = dynamic_cast<IDocumentBoard *>(c);
        return rules->check(id, core->get_board(), cache, status_cb);
    }
    if (auto rules = dynamic_cast<SchematicRules *>(r)) {
        auto core = dynamic_cast<IDocumentSchematic *>(c);
        return rules->check(id, core->get_schematic(), cache);
    }
    if (auto rules = dynamic_cast<PackageRules *>(r)) {
        auto core = dynamic_cast<IDocumentPackage *>(c);
        return rules->check(id, core->get_package(), cache);
    }
    return RulesCheckResult();
}
void rules_apply(Rules *r, RuleID id, class IDocument *c)
{
    if (auto rules = dynamic_cast<BoardRules *>(r)) {
        auto core = dynamic_cast<IDocumentBoard *>(c);
        rules->apply(id, core->get_board(), *core->get_via_padstack_provider());
    }
}
} // namespace horizon
