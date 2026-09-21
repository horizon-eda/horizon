#include "step_export_settings.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"

namespace horizon {

STEPExportSettings::STEPExportSettings(const json &j)
    : filename(j.at("filename").get<std::string>()), prefix(j.at("prefix").get<std::string>()),
      include_3d_models(j.at("include_3d_models")), min_diameter(j.value("min_diameter", 0_mm))
{
    auto excllist = j.value("pkgs_excluded", json::array());

    for (const auto &excluuid : excllist) {
        if (!excluuid.is_string())
            continue;

        pkgs_excluded.emplace(excluuid.get<std::string>());
    }
}

json STEPExportSettings::serialize() const
{
    json j;
    j["filename"] = filename;
    j["prefix"] = prefix;
    j["include_3d_models"] = include_3d_models;
    if (min_diameter)
        j["min_diameter"] = min_diameter;
    if (pkgs_excluded.size()) {
        auto excllist = json::array();

        for (const auto &uuid : pkgs_excluded) {
            excllist.push_back(uuid);
        }

        j["pkgs_excluded"] = excllist;
    }
    return j;
}

} // namespace horizon
