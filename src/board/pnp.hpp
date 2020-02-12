#pragma once
#include "common/lut.hpp"
#include "util/placement.hpp"

namespace horizon {

enum class PnPColumn {
    MPN,
    VALUE,
    MANUFACTURER,
    REFDES,
    PACKAGE,
    X,
    Y,
    ANGLE,
    SIDE,
};

extern const LutEnumStr<PnPColumn> pnp_column_lut;
extern const std::map<PnPColumn, std::string> pnp_column_names;

class PnPRow {
public:
    std::string MPN;
    std::string value;
    std::string manufacturer;
    std::string refdes;
    std::string package;
    Placement placement;

    enum class Side { TOP, BOTTOM };
    Side side;

    std::string get_column(PnPColumn col) const;
};
} // namespace horizon
