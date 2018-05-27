#include "bom_export_settings.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

const LutEnumStr<BOMExportSettings::CSVSettings::Order> bom_order_lut = {
        {"asc", BOMExportSettings::CSVSettings::Order::ASC},
        {"desc", BOMExportSettings::CSVSettings::Order::DESC},
};

BOMExportSettings::BOMExportSettings(const json &j)
    : csv_settings(j.at("csv_settings")), output_filename(j.at("output_filename").get<std::string>())
{
}

json BOMExportSettings::serialize() const
{
    json j;
    j["output_filename"] = output_filename;
    j["csv_settings"] = csv_settings.serialize();
    return j;
}

BOMExportSettings::CSVSettings::CSVSettings(const json &j)
    : sort_column(bom_column_lut.lookup(j.at("sort_column"))), order(bom_order_lut.lookup(j.value("order", "asc")))
{
    for (const auto &it : j.at("columns")) {
        columns.push_back(bom_column_lut.lookup(it));
    }
}

json BOMExportSettings::CSVSettings::serialize() const
{
    json j;
    j["sort_column"] = bom_column_lut.lookup_reverse(sort_column);
    j["order"] = bom_order_lut.lookup_reverse(order);
    j["columns"] = json::array();
    for (const auto &it : columns) {
        j["columns"].push_back(bom_column_lut.lookup_reverse(it));
    }

    return j;
}

} // namespace horizon
