#pragma once
#include <string>
#include <vector>
#include "util/layer_range.hpp"

namespace horizon {
class BoardLayers {
public:
    enum Layer {
        TOP_NOTES = 200,
        OUTLINE_NOTES = 110,
        L_OUTLINE = 100,
        TOP_COURTYARD = 60,
        TOP_ASSEMBLY = 50,
        TOP_PACKAGE = 40,
        TOP_PASTE = 30,
        TOP_SILKSCREEN = 20,
        TOP_MASK = 10,
        TOP_COPPER = 0,
        IN1_COPPER = -1,
        IN2_COPPER = -2,
        IN3_COPPER = -3,
        IN4_COPPER = -4,
        IN5_COPPER = -5,
        IN6_COPPER = -6,
        IN7_COPPER = -7,
        IN8_COPPER = -8,
        BOTTOM_COPPER = -100,
        BOTTOM_MASK = -110,
        BOTTOM_SILKSCREEN = -120,
        BOTTOM_PASTE = -130,
        BOTTOM_PACKAGE = -140,
        BOTTOM_ASSEMBLY = -150,
        BOTTOM_COURTYARD = -160,
        BOTTOM_NOTES = -200
    };

    static const LayerRange layer_range_through;

    static bool is_copper(int l)
    {
        return l <= TOP_COPPER && l >= BOTTOM_COPPER;
    }

    static bool is_copper(const LayerRange &l)
    {
        return is_copper(l.start()) || is_copper(l.end());
    }

    static bool is_silkscreen(int l)
    {
        return l == TOP_SILKSCREEN || l == BOTTOM_SILKSCREEN;
    }

    static const unsigned int max_inner_layers;

    static std::string get_layer_name(int l);
    static const std::vector<int> &get_layers();
};
} // namespace horizon
