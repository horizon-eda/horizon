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
    if (j.count("orderable_MPNs")) {
        const json &o = j["orderable_MPNs"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            orderable_MPNs.emplace(u, it.value().get<std::string>());
        }
    }
}

BOMExportSettings::BOMExportSettings()
{
}

json BOMExportSettings::serialize() const
{
    json j;
    j["output_filename"] = output_filename;
    j["csv_settings"] = csv_settings.serialize();
    j["orderable_MPNs"] = json::object();
    for (const auto &it : orderable_MPNs) {
        j["orderable_MPNs"][(std::string)it.first] = (std::string)it.second;
    }
    return j;
}

BOMExportSettings::CSVSettings::CSVSettings(const json &j)
    : sort_column(bom_column_lut.lookup(j.at("sort_column"))), order(bom_order_lut.lookup(j.value("order", "asc")))
{
    for (const auto &it : j.at("columns")) {
        columns.push_back(bom_column_lut.lookup(it));
    }
}

BOMExportSettings::CSVSettings::CSVSettings()
    : columns({BOMColumn::QTY, BOMColumn::MPN, BOMColumn::VALUE, BOMColumn::MANUFACTURER, BOMColumn::REFDES})
{
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
