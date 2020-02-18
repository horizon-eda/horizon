#include "pnp.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>

namespace horizon {

const std::map<PnPColumn, std::string> pnp_column_names = {
        {PnPColumn::MANUFACTURER, "Manufacturer"},
        {PnPColumn::MPN, "MPN"},
        {PnPColumn::REFDES, "Ref. Des."},
        {PnPColumn::VALUE, "Value"},
        {PnPColumn::PACKAGE, "Package"},
        {PnPColumn::X, "X position"},
        {PnPColumn::Y, "Y position"},
        {PnPColumn::ANGLE, "Angle"},
        {PnPColumn::SIDE, "Side"},
};

const LutEnumStr<PnPColumn> pnp_column_lut = {
        {"manufacturer", PnPColumn::MANUFACTURER},
        {"MPN", PnPColumn::MPN},
        {"refdes", PnPColumn::REFDES},
        {"value", PnPColumn::VALUE},
        {"package", PnPColumn::PACKAGE},
        {"x", PnPColumn::X},
        {"y", PnPColumn::Y},
        {"angle", PnPColumn::ANGLE},
        {"side", PnPColumn::SIDE},
};
static std::string pnp_dim_to_string(int64_t x)
{
    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    ss << std::fixed << std::setprecision(3) << std::internal << (x / 1e6);
    return ss.str();
}

static std::string pnp_angle_to_string(int x)
{
    while (x < 0) {
        x += 65536;
    }
    x %= 65536;

    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    ss << std::fixed << std::setprecision(3) << std::internal << std::abs((x / 65536.0) * 360);
    return ss.str();
}


std::string PnPRow::get_column(PnPColumn col) const
{
    switch (col) {

    case PnPColumn::MANUFACTURER:
        return manufacturer;

    case PnPColumn::MPN:
        return MPN;

    case PnPColumn::VALUE:
        return value;

    case PnPColumn::PACKAGE:
        return package;

    case PnPColumn::REFDES:
        return refdes;

    case PnPColumn::SIDE:
        if (side == Side::TOP)
            return "top";
        else
            return "bottom";

    case PnPColumn::ANGLE:
        return pnp_angle_to_string(placement.get_angle());

    case PnPColumn::X:
        return pnp_dim_to_string(placement.shift.x);

    case PnPColumn::Y:
        return pnp_dim_to_string(placement.shift.y);

    default:
        return "";
    }
}

} // namespace horizon
