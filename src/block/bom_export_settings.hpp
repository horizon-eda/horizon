#pragma once
#include "common/lut.hpp"
#include "nlohmann/json_fwd.hpp"
#include "bom.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

class BOMExportSettings {
public:
    BOMExportSettings(const json &);
    BOMExportSettings()
    {
    }
    json serialize() const;

    enum class Format { CSV };
    Format format = Format::CSV;

    class CSVSettings {
    public:
        CSVSettings(const json &j);
        CSVSettings()
        {
        }

        std::vector<BOMColumn> columns;
        BOMColumn sort_column = BOMColumn::REFDES;
        enum class Order { ASC, DESC };
        Order order = Order::ASC;

        json serialize() const;
    };

    CSVSettings csv_settings;

    std::string output_filename;
};
} // namespace horizon
