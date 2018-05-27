#include "bom.hpp"

namespace horizon {
const std::map<BOMColumn, std::string> bom_column_names = {
        {BOMColumn::DATASHEET, "Datasheet"},
        {BOMColumn::DESCRIPTION, "Description"},
        {BOMColumn::MANUFACTURER, "Manufacturer"},
        {BOMColumn::MPN, "MPN"},
        {BOMColumn::QTY, "QTY"},
        {BOMColumn::REFDES, "Ref. Des."},
        {BOMColumn::VALUE, "Value"},
        {BOMColumn::PACKAGE, "Package"},
};

const LutEnumStr<BOMColumn> bom_column_lut = {
        {"datasheet", BOMColumn::DATASHEET},
        {"description", BOMColumn::DESCRIPTION},
        {"manufacturer", BOMColumn::MANUFACTURER},
        {"MPN", BOMColumn::MPN},
        {"QTY", BOMColumn::QTY},
        {"refdes", BOMColumn::REFDES},
        {"value", BOMColumn::VALUE},
        {"package", BOMColumn::PACKAGE},
};

std::string BOMRow::get_column(BOMColumn col) const
{
    switch (col) {
    case BOMColumn::DATASHEET:
        return datasheet;

    case BOMColumn::DESCRIPTION:
        return description;

    case BOMColumn::MANUFACTURER:
        return manufacturer;

    case BOMColumn::MPN:
        return MPN;

    case BOMColumn::QTY:
        return std::to_string(refdes.size());

    case BOMColumn::VALUE:
        return value;

    case BOMColumn::PACKAGE:
        return package;

    case BOMColumn::REFDES: {
        std::string s;
        for (const auto &it : refdes) {
            s += it;
            s += ", ";
        }
        s.pop_back();
        s.pop_back();
        return s;
    }

    default:
        return "";
    }
}

} // namespace horizon
