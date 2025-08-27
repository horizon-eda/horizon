#pragma once
#include <nlohmann/json.hpp>
#include "util/uuid.hpp"
#include "rules/rules_import_export.hpp"

namespace horizon {
using json = nlohmann::json;
class BoardRulesImportInfo : public RulesImportInfo {
public:
    BoardRulesImportInfo(const json &j);
    unsigned int n_inner_layers;
    std::map<UUID, std::string> net_classes;
    UUID net_class_default;
    json rules;
    const json &get_rules() const override
    {
        return rules;
    }
};

} // namespace horizon