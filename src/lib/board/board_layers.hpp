#pragma once

namespace horizon {
class BoardLayers {
public:
    enum Layer {
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
        BOTTOM_COPPER = -100,
        BOTTOM_MASK = -110,
        BOTTOM_SILKSCREEN = -120,
        BOTTOM_PASTE = -130,
        BOTTOM_PACKAGE = -140,
        BOTTOM_ASSEMBLY = -150,
        BOTTOM_COURTYARD = -160,
    };

    static bool is_copper(int l)
    {
        return l <= TOP_COPPER && l >= BOTTOM_COPPER;
    }
};
} // namespace horizon
