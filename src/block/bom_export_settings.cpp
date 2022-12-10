#include "bom_export_settings.hpp"
#include "nlohmann/json.hpp"
#include "pool/ipool.hpp"
#include "pool/part.hpp"

namespace horizon {

const LutEnumStr<BOMExportSettings::CSVSettings::Order> bom_order_lut = {
        {"asc", BOMExportSettings::CSVSettings::Order::ASC},
        {"desc", BOMExportSettings::CSVSettings::Order::DESC},
};

BOMExportSettings::BOMExportSettings(const json &j, IPool &pool)
    : csv_settings(j.at("csv_settings")), output_filename(j.at("output_filename").get<std::string>()),
      include_nopopulate(j.value("include_nopopulate", true))
{
    if (j.count("orderable_MPNs")) {
        for (const auto &[key, value] : j.at("orderable_MPNs").items()) {
            orderable_MPNs.emplace(key, value.get<std::string>());
        }
    }
    if (j.count("concrete_parts")) {
        for (const auto &[key, value] : j.at("concrete_parts").items()) {
            std::shared_ptr<const Part> part = nullptr;
            try {
                part = pool.get_part(value.get<std::string>());
            }
            catch (...) {
                // not found
            }
            if (part)
                concrete_parts.emplace(key, part);
        }
    }
}

void BOMExportSettings::update_refs(IPool &pool)
{
    for (auto &[k, v] : concrete_parts) {
        v = pool.get_part(v->uuid);
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
    j["concrete_parts"] = json::object();
    for (const auto &it : concrete_parts) {
        j["concrete_parts"][(std::string)it.first] = (std::string)it.second->uuid;
    }
    if (!include_nopopulate)
        j["include_nopopulate"] = false;
    return j;
}

BOMExportSettings::CSVSettings::CSVSettings(const json &j)
    : sort_column(bom_column_lut.lookup(j.at("sort_column"))), order(bom_order_lut.lookup(j.value("order", "asc"))),
      custom_column_names(j.value("custom_column_names", false))
{
    for (const auto &it : j.at("columns")) {
        columns.push_back(bom_column_lut.lookup(it));
    }
    if (j.count("column_names")) {
        for (const auto &[col, name] : j.at("column_names").items()) {
            column_names.emplace(bom_column_lut.lookup(col), name);
        }
    }
    else {
        for (const auto &[col, name] : bom_column_names) {
            column_names.emplace(col, name);
        }
    }
}

const std::string &BOMExportSettings::CSVSettings::get_column_name(BOMColumn col) const
{
    if (custom_column_names && column_names.count(col))
        return column_names.at(col);
    else
        return bom_column_names.at(col);
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
    if (custom_column_names) {
        j["custom_column_names"] = true;
        j["column_names"] = json::object();
        for (const auto &[col, name] : column_names) {
            j["column_names"][bom_column_lut.lookup_reverse(col)] = name;
        }
    }

    return j;
}

} // namespace horizon
