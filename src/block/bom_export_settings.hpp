#pragma once
#include "common/lut.hpp"
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "bom.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

class BOMExportSettings {
public:
    BOMExportSettings(const json &, class Pool &pool);
    BOMExportSettings();
    json serialize() const;

    enum class Format { CSV };
    Format format = Format::CSV;
    std::map<UUID, UUID> orderable_MPNs;               // part->orderable MPN
    std::map<UUID, const class Part *> concrete_parts; // part->part

    class CSVSettings {
    public:
        CSVSettings(const json &j);
        CSVSettings();

        std::vector<BOMColumn> columns;
        BOMColumn sort_column = BOMColumn::REFDES;
        enum class Order { ASC, DESC };
        Order order = Order::ASC;

        json serialize() const;
    };

    CSVSettings csv_settings;

    std::string output_filename;

    bool include_nopopulate = true;
};
} // namespace horizon
