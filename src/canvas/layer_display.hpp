#pragma once
#include "common/common.hpp"

namespace horizon {
class LayerDisplay {
public:
    enum class Mode { OUTLINE, HATCH, FILL, FILL_ONLY, N_MODES };
    LayerDisplay(bool vi, Mode mo) : visible(vi), mode(mo)
    {
    }
    LayerDisplay()
    {
    }

    bool visible = true;
    Mode mode = Mode::FILL;
    uint32_t types_visible = 0xffffffff; // bit mask of Triangle::Type
    uint32_t types_force_outline = 0;    // bit mask of Triangle::Type
};
} // namespace horizon
