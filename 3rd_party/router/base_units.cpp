#include "base_units.h"
#include "util/geom_util.hpp"

wxString MessageTextFromValue(EDA_UNITS aUnits, int aValue)
{
    if (aUnits == EDA_UNITS::MILLIMETRES) {
        return horizon::dim_to_string(aValue);
    }
    return "";
}
