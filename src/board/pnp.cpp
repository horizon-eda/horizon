#include "pnp.hpp"
#include "pnp_export_settings.hpp"
#include "util/geom_util.hpp"
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

static std::string fmt_pos(const std::string &fmt, int64_t x)
{
    std::string out;
    enum class State { NORMAL, FMT_START, FMT_NDEC, FMT_UNIT };
    State state = State::NORMAL;
    unsigned int n_dec = 3;

    for (const auto c : fmt) {
        switch (state) {
        case State::NORMAL:
            if (c == '%') {
                state = State::FMT_START;
            }
            else {
                out.append(1, c);
            }
            break;

        case State::FMT_START:
            if (c != '.') {
                return "Format error, . must follow %";
            }
            else {
                state = State::FMT_NDEC;
            }
            break;

        case State::FMT_NDEC:
            if (isdigit(c)) {
                n_dec = c - '0';
                state = State::FMT_UNIT;
            }
            else {
                return "Format error, not a digit";
            }
            break;

        case State::FMT_UNIT: {
            double multiplier = 1e-6;
            switch (c) {
            case 'm':
                multiplier = 1e-6;
                break;

            case 'u':
                multiplier = 1e-3;
                break;

            case 'i':
                multiplier = 1e-6 / 25.4;
                break;

            case 't':
                multiplier = (1e-6 / 25.4) * 1e3;
                break;

            default:
                return "Format error, unsupported unit";
            }

            std::ostringstream ss;
            ss.imbue(std::locale::classic());
            ss << std::fixed << std::setprecision(n_dec) << std::internal << (x * multiplier);
            out.append(ss.str());
            state = State::NORMAL;
        } break;
        }
    }

    return out;
}

static std::string pnp_angle_to_string(int x)
{
    x = wrap_angle(x);

    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    ss << std::fixed << std::setprecision(3) << std::internal << std::abs((x / 65536.0) * 360);
    return ss.str();
}

static std::string pnp_dim_to_string(int64_t x)
{
    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    ss << std::fixed << std::setprecision(3) << std::internal << (x / 1e6);
    return ss.str();
}

static int64_t get_dim(const Placement &pl, PnPColumn col)
{
    if (col == PnPColumn::X)
        return pl.shift.x;
    else
        return pl.shift.y;
}

std::string PnPRow::get_column(PnPColumn col, const PnPExportSettings &settings) const
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
        if (side == Side::TOP) {
            if (settings.customize)
                return settings.top_side;
            else
                return "top";
        }
        else {
            if (settings.customize)
                return settings.bottom_side;
            else
                return "bottom";
        }

    case PnPColumn::ANGLE:
        return pnp_angle_to_string(placement.get_angle());

    case PnPColumn::X:
    case PnPColumn::Y: {
        const auto d = get_dim(placement, col);
        if (settings.customize)
            return fmt_pos(settings.position_format, d);
        else
            return pnp_dim_to_string(d);
    } break;

    default:
        return "";
    }
}

} // namespace horizon
