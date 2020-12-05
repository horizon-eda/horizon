#pragma once
#include "rules.hpp"
#include "rules_import_export.hpp"
#include "nlohmann/json_fwd.hpp"
#include <memory>

namespace horizon {
using json = nlohmann::json;
RulesCheckResult rules_check(Rules &rules, RuleID id, class IDocument &c, class RulesCheckCache &cache,
                             check_status_cb_t status_cb);
void rules_apply(const Rules &rules, RuleID id, class IDocument &c);
json rules_export(const Rules &rules, const class RulesExportInfo &export_info, IDocument &c);
std::unique_ptr<RulesImportInfo> rules_get_import_info(const json &j);
} // namespace horizon
