#include "board_rules_import.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
BoardRulesImportInfo::BoardRulesImportInfo(const json &j)
    : RulesImportInfo(j), n_inner_layers(j.at("n_inner_layers")),
      net_class_default(j.at("net_class_default").get<std::string>()), rules(j.at("rules"))
{
    for (const auto &[key, value] : j.at("net_classes").items()) {
        const UUID uu(key);
        net_classes.emplace(uu, value.get<std::string>());
    }
}
} // namespace horizon
