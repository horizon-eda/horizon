#pragma once
#include "common/common.hpp"
#include "triangle.hpp"

namespace horizon {
class LayerDisplay {
public:
    // also used in shaders
    enum class Mode { OUTLINE = 0, HATCH = 1, FILL = 2, FILL_ONLY = 3, DOTTED = 4, N_MODES };
    LayerDisplay(bool vi, Mode mo);
    LayerDisplay();

    bool visible = true;
    Mode mode = Mode::FILL;
    uint32_t types_visible = 0xffffffff; // bit mask of Triangle::Type
    bool type_is_visible(TriangleInfo::Type type) const;
};
} // namespace horizon
