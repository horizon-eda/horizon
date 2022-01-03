#pragma once
#include "lut.hpp"

namespace horizon {

enum class PinNameOrientation { IN_LINE, PERPENDICULAR, HORIZONTAL };
extern const LutEnumStr<PinNameOrientation> pin_name_orientation_lut;

} // namespace horizon
