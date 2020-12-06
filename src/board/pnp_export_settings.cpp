#include "pnp_export_settings.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

const LutEnumStr<PnPExportSettings::Mode> PnPExportSettings::mode_lut = {
        {"merged", PnPExportSettings::Mode::MERGED},
        {"individual", PnPExportSettings::Mode::INDIVIDUAL},
};

PnPExportSettings::PnPExportSettings(const json &j)
    : mode(mode_lut.lookup(j.at("mode"))), include_nopopulate(j.value("include_nopopulate", true)),
      customize(j.value("customize", false)), position_format(j.value("position_format", "%.3m")),
      top_side(j.value("top_side", "top")), bottom_side(j.value("bottom_side", "bottom")),
      output_directory(j.at("output_directory").get<std::string>()),
      filename_top(j.at("filename_top").get<std::string>()),
      filename_bottom(j.at("filename_bottom").get<std::string>()),
      filename_merged(j.at("filename_merged").get<std::string>())
{
    for (const auto &it : j.at("columns")) {
        columns.push_back(pnp_column_lut.lookup(it));
    }
    if (j.count("column_names")) {
        for (const auto &[col, name] : j.at("column_names").items()) {
            column_names.emplace(pnp_column_lut.lookup(col), name);
        }
    }
    else {
        for (const auto &[col, name] : pnp_column_names) {
            column_names.emplace(col, name);
        }
    }
}

PnPExportSettings::PnPExportSettings()
    : columns({PnPColumn::REFDES, PnPColumn::X, PnPColumn::Y, PnPColumn::ANGLE, PnPColumn::SIDE})
{
}

const std::string &PnPExportSettings::get_column_name(PnPColumn col) const
{
    if (customize && column_names.count(col))
        return column_names.at(col);
    else
        return pnp_column_names.at(col);
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
    j["customize"] = customize;
    j["position_format"] = position_format;
    j["top_side"] = top_side;
    j["bottom_side"] = bottom_side;
    j["column_names"] = json::object();
    for (const auto &[col, name] : column_names) {
        j["column_names"][pnp_column_lut.lookup_reverse(col)] = name;
    }

    return j;
}

} // namespace horizon
