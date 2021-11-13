#include "layer_display.hpp"
#include "triangle.hpp"

namespace horizon {
static const uint32_t types_visible_default = ~(1 << static_cast<uint32_t>(TriangleInfo::Type::KEEPOUT_FILL));

LayerDisplay::LayerDisplay() : types_visible(types_visible_default)
{
}

LayerDisplay::LayerDisplay(bool vi, Mode mo) : visible(vi), mode(mo), types_visible(types_visible_default)
{
}
} // namespace horizon
