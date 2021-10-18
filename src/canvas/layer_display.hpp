#pragma once
#include "common/common.hpp"

namespace horizon {
class LayerDisplay {
public:
    // also used in shaders
    enum class Mode { OUTLINE = 0, HATCH = 1, FILL = 2, FILL_ONLY = 3, DOTTED = 4, N_MODES };
    LayerDisplay(bool vi, Mode mo) : visible(vi), mode(mo)
    {
    }
    LayerDisplay()
    {
    }

    bool visible = true;
    Mode mode = Mode::FILL;
    uint32_t types_visible = 0xffffffff; // bit mask of Triangle::Type
};
} // namespace horizon
