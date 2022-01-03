#include "pin_name_orientation.hpp"

namespace horizon {
const LutEnumStr<PinNameOrientation> pin_name_orientation_lut = {
        {"in_line", PinNameOrientation::IN_LINE},
        {"perpendicular", PinNameOrientation::PERPENDICULAR},
        {"horizontal", PinNameOrientation::HORIZONTAL},
};
}
