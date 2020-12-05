#include "rules_import_export.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

RulesImportInfo::RulesImportInfo(const json &j) : name(j.at("name")), notes(j.at("notes"))
{
}

RulesExportInfo::RulesExportInfo(const json &j)
    : name(j.at("name")), notes(j.at("notes")), uuid(j.at("uuid").get<std::string>())
{
}

RulesExportInfo::RulesExportInfo() : uuid(UUID::random())
{
}

void RulesExportInfo::serialize(json &j) const
{
    j["name"] = name;
    j["notes"] = notes;
    j["uuid"] = (std::string)uuid;
}
} // namespace horizon
