#include "appearance.hpp"
#include "board/board_layers.hpp"

namespace horizon {

Appearance::Appearance()
{
    colors[ColorP::JUNCTION] = {1, 1, 0};
    colors[ColorP::FRAG_ORPHAN] = {1, 1, 0};
    colors[ColorP::AIRWIRE_ROUTER] = {1, 1, 0};
    colors[ColorP::TEXT_OVERLAY] = {1, 1, 1};
    colors[ColorP::HOLE] = {1, 1, 1};
    colors[ColorP::DIMENSION] = {1, 1, 1};
    colors[ColorP::ERROR] = {1, 0, 0};
    colors[ColorP::NET] = {0, 1, 0};
    colors[ColorP::BUS] = {1, .4, 0};
    colors[ColorP::FRAME] = {0, .5, 0};
    colors[ColorP::AIRWIRE] = {0, 1, 1};
    colors[ColorP::PIN] = {1, 1, 1};
    colors[ColorP::PIN_HIDDEN] = {.5, .5, .5};
    colors[ColorP::DIFFPAIR] = {.5, 1, 0};
    colors[ColorP::CURSOR_NORMAL] = {0, 1, 0};
    colors[ColorP::CURSOR_TARGET] = {1, 0, 0};
    colors[ColorP::BACKGROUND] = Color::new_from_int(0, 24, 64);
    colors[ColorP::GRID] = Color::new_from_int(0, 78, 208);
    colors[ColorP::ORIGIN] = {0, 1, 0};
    colors[ColorP::MARKER_BORDER] = {1, 1, 1};
    colors[ColorP::SELECTION_BOX] = {1, 0, 0};
    colors[ColorP::SELECTION_LINE] = {0, 1, 1};
    colors[ColorP::SELECTABLE_OUTER] = {1, 0, 1};
    colors[ColorP::SELECTABLE_INNER] = {0, 0, 0};
    colors[ColorP::SELECTABLE_ALWAYS] = {1, 1, 0};
    colors[ColorP::SELECTABLE_PRELIGHT] = {.5, 0, .5};
    colors[ColorP::SEARCH] = {0, 0, 1};
    colors[ColorP::SEARCH_CURRENT] = {1, 0, 1};
    colors[ColorP::SHADOW] = {.3, .3, .3};
    colors[ColorP::CONNECTION_LINE] = {.7, 0, .6};
    colors[ColorP::NOPOPULATE_X] = {.8, .4, .4};

    layer_colors[BoardLayers::TOP_NOTES] = {1, 1, 1};
    layer_colors[BoardLayers::OUTLINE_NOTES] = {.6, .6, 0};
    layer_colors[BoardLayers::L_OUTLINE] = {.6, .6, 0};
    layer_colors[BoardLayers::TOP_COURTYARD] = {.5, .5, .5};
    layer_colors[BoardLayers::TOP_ASSEMBLY] = {.5, .5, .5};
    layer_colors[BoardLayers::TOP_PACKAGE] = {.5, .5, .5};
    layer_colors[BoardLayers::TOP_PASTE] = {.8, .8, .8};
    layer_colors[BoardLayers::TOP_SILKSCREEN] = {.9, .9, .9};
    layer_colors[BoardLayers::TOP_MASK] = {1, .5, .5};
    layer_colors[BoardLayers::TOP_COPPER] = {1, 0, 0};
    layer_colors[BoardLayers::IN1_COPPER] = {1, 1, 0};
    layer_colors[BoardLayers::IN2_COPPER] = {1, 1, 0};
    layer_colors[BoardLayers::IN3_COPPER] = {1, 1, 0};
    layer_colors[BoardLayers::IN4_COPPER] = {1, 1, 0};
    layer_colors[BoardLayers::BOTTOM_COPPER] = {0, .5, 0};
    layer_colors[BoardLayers::BOTTOM_MASK] = {.25, .5, .25};
    layer_colors[BoardLayers::BOTTOM_SILKSCREEN] = {.9, .9, .9};
    layer_colors[BoardLayers::BOTTOM_PASTE] = {.8, .8, .8};
    layer_colors[BoardLayers::BOTTOM_PACKAGE] = {.5, .5, .5};
    layer_colors[BoardLayers::BOTTOM_ASSEMBLY] = {.5, .5, .5};
    layer_colors[BoardLayers::BOTTOM_COURTYARD] = {.5, .5, .5};

    layer_colors[BoardLayers::BOTTOM_NOTES] = {1, 1, 1};
    layer_colors[10000] = {1, 1, 1};
}
} // namespace horizon
