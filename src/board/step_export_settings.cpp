#include "step_export_settings.hpp"
#include <nlohmann/json.hpp>
#include "util/util.hpp"

namespace horizon {

STEPExportSettings::STEPExportSettings(const json &j)
    : filename(j.at("filename").get<std::string>()), prefix(j.at("prefix").get<std::string>()),
      include_3d_models(j.at("include_3d_models")), min_diameter(j.value("min_diameter", 0_mm))
{
}

json STEPExportSettings::serialize() const
{
    json j;
    j["filename"] = filename;
    j["prefix"] = prefix;
    j["include_3d_models"] = include_3d_models;
    if (min_diameter)
        j["min_diameter"] = min_diameter;
    return j;
}

} // namespace horizon
