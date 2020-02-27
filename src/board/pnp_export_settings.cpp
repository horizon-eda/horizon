#include "pnp_export_settings.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

const LutEnumStr<PnPExportSettings::Mode> PnPExportSettings::mode_lut = {
        {"merged", PnPExportSettings::Mode::MERGED},
        {"individual", PnPExportSettings::Mode::INDIVIDUAL},
};

PnPExportSettings::PnPExportSettings(const json &j)
    : mode(mode_lut.lookup(j.at("mode"))), include_nopopulate(j.value("include_nopopulate", true)),
      output_directory(j.at("output_directory").get<std::string>()),
      filename_top(j.at("filename_top").get<std::string>()),
      filename_bottom(j.at("filename_bottom").get<std::string>()),
      filename_merged(j.at("filename_merged").get<std::string>())
{
    for (const auto &it : j.at("columns")) {
        columns.push_back(pnp_column_lut.lookup(it));
    }
}

PnPExportSettings::PnPExportSettings()
    : columns({PnPColumn::REFDES, PnPColumn::X, PnPColumn::Y, PnPColumn::ANGLE, PnPColumn::SIDE})
{
}

json PnPExportSettings::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(mode);
    if (!include_nopopulate) {
        j["include_nopopulate"] = false;
    }
    j["output_directory"] = output_directory;
    j["filename_top"] = filename_top;
    j["filename_bottom"] = filename_bottom;
    j["filename_merged"] = filename_merged;
    j["columns"] = json::array();
    for (const auto &it : columns) {
        j["columns"].push_back(pnp_column_lut.lookup_reverse(it));
    }

    return j;
}

} // namespace horizon
