#pragma once
#include "common/lut.hpp"
#include <vector>

namespace horizon {

enum class BOMColumn {
    QTY,
    MPN,
    VALUE,
    MANUFACTURER,
    REFDES,
    DESCRIPTION,
    DATASHEET,
    PACKAGE,
};

extern const LutEnumStr<BOMColumn> bom_column_lut;
extern const std::map<BOMColumn, std::string> bom_column_names;

class BOMRow {
public:
    std::string MPN;
    std::string manufacturer;
    std::vector<std::string> refdes;
    std::string datasheet;
    std::string description;
    std::string value;
    std::string package;

    std::string get_column(BOMColumn col) const;
};
} // namespace horizon
