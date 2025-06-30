#include "rules_with_core.hpp"
#include "board/board_rules.hpp"
#include "board/board_rules_import.hpp"
#include "document/idocument_board.hpp"
#include "document/idocument_schematic.hpp"
#include "document/idocument_package.hpp"
#include "document/idocument_symbol.hpp"
#include "schematic/schematic_rules.hpp"
#include "package/package_rules.hpp"
#include "symbol/symbol_rules.hpp"
#include <nlohmann/json.hpp>

namespace horizon {
RulesCheckResult rules_check(Rules &r, RuleID id, class IDocument &c, RulesCheckCache &cache,
                             check_status_cb_t status_cb, const std::atomic_bool &cancel)
{
    if (auto rules = dynamic_cast<BoardRules *>(&r)) {
        auto &core = dynamic_cast<IDocumentBoard &>(c);
        return rules->check(id, *core.get_board(), cache, status_cb, cancel);
    }
    if (auto rules = dynamic_cast<SchematicRules *>(&r)) {
        auto &core = dynamic_cast<IDocumentSchematic &>(c);
        return rules->check(id, core.get_blocks(), cache);
    }
    if (auto rules = dynamic_cast<PackageRules *>(&r)) {
        auto &core = dynamic_cast<IDocumentPackage &>(c);
        return rules->check(id, core.get_package());
    }
    if (auto rules = dynamic_cast<SymbolRules *>(&r)) {
        auto &core = dynamic_cast<IDocumentSymbol &>(c);
        return rules->check(id, core.get_symbol());
    }
    return RulesCheckResult();
}
void rules_apply(const Rules &r, RuleID id, class IDocument &c)
{
    if (auto rules = dynamic_cast<const BoardRules *>(&r)) {
        auto &core = dynamic_cast<IDocumentBoard &>(c);
        rules->apply(id, *core.get_board(), core.get_pool_caching());
    }
}

json rules_export(const Rules &r, const class RulesExportInfo &export_info, IDocument &c)
{
    if (auto rules = dynamic_cast<const BoardRules *>(&r)) {
        auto &core = dynamic_cast<IDocumentBoard &>(c);
        return rules->export_rules(export_info, *core.get_board());
    }
    throw std::runtime_error("not supported");
}

std::unique_ptr<RulesImportInfo> rules_get_import_info(const json &j)
{
    if (j.at("rules_type") == "board") {
        return std::make_unique<BoardRulesImportInfo>(j);
    }
    else {
        throw std::runtime_error("unsupported rules");
    }
}

} // namespace horizon
