#include "board_layers.hpp"

namespace horizon {
std::string BoardLayers::get_layer_name(int l)
{
    switch (l) {
    case TOP_NOTES:
        return "Top Notes";

    case OUTLINE_NOTES:
        return "Outline Notes";

    case L_OUTLINE:
        return "Outline";

    case TOP_COURTYARD:
        return "Top Courtyard";

    case TOP_ASSEMBLY:
        return "Top Assembly";

    case TOP_PACKAGE:
        return "Top Package";

    case TOP_PASTE:
        return "Top Paste";

    case TOP_SILKSCREEN:
        return "Top Silkscreen";

    case TOP_MASK:
        return "Top Mask";

    case TOP_COPPER:
        return "Top Copper";

    case IN1_COPPER:
    case IN2_COPPER:
    case IN3_COPPER:
    case IN4_COPPER:
        return "Inner " + std::to_string(-l);

    case BOTTOM_COPPER:
        return "Bottom Copper";

    case BOTTOM_MASK:
        return "Bottom Mask";

    case BOTTOM_SILKSCREEN:
        return "Bottom Silkscreen";

    case BOTTOM_PASTE:
        return "Bottom Paste";

    case BOTTOM_PACKAGE:
        return "Bottom Package";

    case BOTTOM_ASSEMBLY:
        return "Bottom Assembly";

    case BOTTOM_COURTYARD:
        return "Bottom Courtyard";

    case BOTTOM_NOTES:
        return "Bottom Notes";
    }
    return "Invalid layer " + std::to_string(l);
}

static const std::vector<int> layers = {
        BoardLayers::TOP_NOTES,      BoardLayers::OUTLINE_NOTES,     BoardLayers::L_OUTLINE,
        BoardLayers::TOP_COURTYARD,  BoardLayers::TOP_ASSEMBLY,      BoardLayers::TOP_PACKAGE,
        BoardLayers::TOP_PASTE,      BoardLayers::TOP_SILKSCREEN,    BoardLayers::TOP_MASK,
        BoardLayers::TOP_COPPER,     BoardLayers::IN1_COPPER,        BoardLayers::IN2_COPPER,
        BoardLayers::IN3_COPPER,     BoardLayers::IN4_COPPER,        BoardLayers::BOTTOM_COPPER,
        BoardLayers::BOTTOM_MASK,    BoardLayers::BOTTOM_SILKSCREEN, BoardLayers::BOTTOM_PASTE,
        BoardLayers::BOTTOM_PACKAGE, BoardLayers::BOTTOM_ASSEMBLY,   BoardLayers::BOTTOM_COURTYARD,
        BoardLayers::BOTTOM_NOTES};

const std::vector<int> &BoardLayers::get_layers()
{
    return layers;
}

} // namespace horizon
